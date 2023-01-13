#include <string.h>
#include "core_dump_binary.h"
#include "ticos/panics/coredump.h"
#include "esp_core_dump_port.h"
#include "esp_core_dump_common.h"
#include "core_dump_checksum.h"

const static DRAM_ATTR char TAG[] __attribute__((unused)) = "esp_core_dump_binary";

/**
 * @brief In the menconfig, it is possible to specify a specific stack size for
 * core dump generation.
 */
#if CONFIG_ESP_COREDUMP_STACK_SIZE > 0

/**
 * @brief If stack size has been specified for the core dump generation, create
 * a stack that will be used during the whole core dump generation.
 */
#if LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG
    /* Increase stack size in verbose mode */
    #define ESP_COREDUMP_STACK_SIZE (CONFIG_ESP_COREDUMP_STACK_SIZE+100)
#else
    #define ESP_COREDUMP_STACK_SIZE CONFIG_ESP_COREDUMP_STACK_SIZE
#endif


#define COREDUMP_STACK_FILL_BYTE (0xa5U)

static uint8_t s_coredump_stack[ESP_COREDUMP_STACK_SIZE];
static uint8_t* s_core_dump_sp = NULL;
static uint8_t* s_core_dump_backup = NULL;

/**
 * @brief Function setting up the core dump stack.
 *
 * @note This function **must** be aligned as it modifies the
 * stack pointer register.
 */
FORCE_INLINE_ATTR void esp_core_dump_setup_stack(void)
{
    s_core_dump_sp = (uint8_t *)((uint32_t)(s_coredump_stack + ESP_COREDUMP_STACK_SIZE - 1) & ~0xf);
    memset(s_coredump_stack, COREDUMP_STACK_FILL_BYTE, ESP_COREDUMP_STACK_SIZE);

    /* watchpoint 1 can be used for task stack overflow detection, re-use it, it is no more necessary */
	//esp_cpu_clear_watchpoint(1);
	//esp_cpu_set_watchpoint(1, s_coredump_stack, 1, ESP_WATCHPOINT_STORE);

    /* Replace the stack pointer depending on the architecture, but save the
     * current stack pointer, in order to be able too restore it later.
     * This function must be inlined. */
    s_core_dump_backup = esp_core_dump_replace_sp(s_core_dump_sp);
    ESP_COREDUMP_LOGI("Backing up stack @ %p and use core dump stack @ %p",
                      s_core_dump_backup, esp_cpu_get_sp());
}

/**
 * @brief Calculate how many bytes are free on the stack set up earlier.
 *
 * @return Size, in bytes, of the available space on the stack.
 */
FORCE_INLINE_ATTR uint32_t esp_core_dump_free_stack_space(const uint8_t *pucStackByte)
{
    uint32_t ulCount = 0U;
    while ( ulCount < ESP_COREDUMP_STACK_SIZE &&
           *pucStackByte == (uint8_t)COREDUMP_STACK_FILL_BYTE )
    {
        pucStackByte -= portSTACK_GROWTH;
        ulCount++;
    }
    ulCount /= sizeof(uint8_t);
    return ulCount;
}

/**
 * @brief Print how many bytes have been used on the stack to create the core
 * dump.
 */
FORCE_INLINE_ATTR void esp_core_dump_report_stack_usage(void)
{
    uint32_t bytes_free = esp_core_dump_free_stack_space(s_coredump_stack);
    ESP_COREDUMP_LOGI("Core dump used %u bytes on stack. %u bytes left free.",
        s_core_dump_sp - s_coredump_stack - bytes_free, bytes_free);

    /* Restore the stack pointer. */
    ESP_COREDUMP_LOGI("Restoring stack @ %p", s_core_dump_backup);
    esp_core_dump_replace_sp(s_core_dump_backup);
}

#else
FORCE_INLINE_ATTR void esp_core_dump_setup_stack(void)
{
    /* If we are in ISR set watchpoint to the end of ISR stack */
    if (esp_core_dump_in_isr_context()) {
        uint8_t* topStack = esp_core_dump_get_isr_stack_top();
        esp_cpu_clear_watchpoint(1);
        esp_cpu_set_watchpoint(1, topStack+xPortGetCoreID()*configISR_STACK_SIZE, 1, ESP_WATCHPOINT_STORE);
    } else {
        /* for tasks user should enable stack overflow detection in menuconfig
        TODO: if not enabled in menuconfig enable it ourselves */
    }
}


FORCE_INLINE_ATTR void esp_core_dump_report_stack_usage(void)
{
}
#endif

static core_dump_write_config_t wr_cfg = { 0 };
static core_dump_write_data_t wr_data = { 0 };

bool proxy_platform_coredump_write(void *data, size_t len, sTcsCoredumpWriteCtx *write_ctx){
		prv_platform_coredump_write(data, len, write_ctx);
		esp_core_dump_checksum_update(wr_data.checksum_ctx, data, len);
		return true;
}


static esp_err_t esp_core_dump_save_task(sTcsCoredumpWriteCtx *write_ctx,
                                         core_dump_task_header_t *task)
{
    uint32_t stk_vaddr = 0;
    uint32_t stk_paddr = 0;
    uint32_t stk_len = esp_core_dump_get_stack(task, &stk_vaddr, &stk_paddr);

    stk_len = esp_core_dump_get_memory_len(stk_vaddr, stk_vaddr+stk_len);

    // Save memory segment header
		proxy_platform_coredump_write((void*)task, sizeof(core_dump_task_header_t), write_ctx);
    // Save TCB block
		proxy_platform_coredump_write(task->tcb_addr, esp_core_dump_get_tcb_len(), write_ctx);
    // Save task stack
		proxy_platform_coredump_write((void*)stk_paddr, stk_len, write_ctx);

    ESP_COREDUMP_LOG_PROCESS("Cust Task (TCB:%x) dump is saved.",
                                    task->tcb_addr);
    return ESP_OK;
}

static esp_err_t esp_core_dump_save_mem_segment(sTcsCoredumpWriteCtx *write_ctx,
                                                core_dump_mem_seg_header_t* seg)
{

    if (!esp_core_dump_mem_seg_is_sane(seg->start, seg->size)) {
        ESP_COREDUMP_LOGE("Failed to write memory segment, (%x, %lu)!",
                                seg->start, seg->size);
        return ESP_FAIL;
    }
    // Save TCB address, stack base and stack top addr
		proxy_platform_coredump_write((void*)seg, sizeof(core_dump_mem_seg_header_t), write_ctx);
    // Save memory contents
		proxy_platform_coredump_write((void*)seg->start, seg->size, write_ctx);
    ESP_COREDUMP_LOG_PROCESS("Memory segment (%x, %lu) is saved.",
                                seg->start, seg->size);
    return ESP_OK;
}

size_t calculate_coredump_bin_size(){
    uint32_t tcb_sz = esp_core_dump_get_tcb_len();
    uint32_t data_len = 0;
    uint32_t bad_tasks_num = 0;
    core_dump_header_t hdr = { 0 };
    core_dump_task_header_t task_hdr = { 0 };
    core_dump_mem_seg_header_t mem_seg = { 0 };
    void *task = NULL;
    void *cur_task = NULL;

    // Verifies all tasks in the snapshot
    esp_core_dump_reset_tasks_snapshots_iter();
    while ((task = esp_core_dump_get_next_task(task))) {
        if (!esp_core_dump_get_task_snapshot(task, &task_hdr, &mem_seg)) {
            bad_tasks_num++;
            continue;
        }
        hdr.tasks_num++;
        if (task == esp_core_dump_get_current_task_handle()) {
            cur_task = task;
            ESP_COREDUMP_LOG_PROCESS("Task %x %x is first crashed task.", cur_task, task_hdr.tcb_addr);
        }
        ESP_COREDUMP_LOG_PROCESS("Stack len = %lu (%x %x)", task_hdr.stack_end-task_hdr.stack_start,
                                    task_hdr.stack_start, task_hdr.stack_end);
        // Increase core dump size by task stack size
        uint32_t stk_vaddr = 0;
        uint32_t stk_paddr = 0;
        uint32_t stk_len = esp_core_dump_get_stack(&task_hdr, &stk_vaddr, &stk_paddr);
        data_len += esp_core_dump_get_memory_len(stk_vaddr, stk_vaddr+stk_len);
        // Add tcb size
        data_len += (tcb_sz + sizeof(core_dump_task_header_t));
        if (mem_seg.size > 0) {
            ESP_COREDUMP_LOG_PROCESS("Add interrupted task stack %lu bytes @ %x",
                    mem_seg.size, mem_seg.start);
            data_len += esp_core_dump_get_memory_len(mem_seg.start, mem_seg.start+mem_seg.size);
            data_len += sizeof(core_dump_mem_seg_header_t);
            hdr.mem_segs_num++;
        }
    }
    ESP_COREDUMP_LOGI("Cal round Found tasks: good %d, bad %d, mem segs %d", hdr.tasks_num, bad_tasks_num, hdr.mem_segs_num);

    // Check if current task TCB is broken
    if (cur_task == NULL) {
        ESP_COREDUMP_LOG_PROCESS("The current crashed task is broken.");
        cur_task = esp_core_dump_get_next_task(NULL);
        if (cur_task == NULL) {
            ESP_COREDUMP_LOGE("No valid tasks in the system!");
            return false;
        }
    }

    // Add user memory regions data size
    for (coredump_region_t i = COREDUMP_MEMORY_START; i < COREDUMP_MEMORY_MAX; i++) {
        uint32_t start = 0;
        int data_sz = esp_core_dump_get_user_ram_info(i, &start);
        if (data_sz < 0) {
            ESP_COREDUMP_LOGE("Invalid memory segment size!");
            return false;
        }
        if (data_sz > 0) {
            hdr.mem_segs_num++;
            data_len += sizeof(core_dump_mem_seg_header_t) + esp_core_dump_get_memory_len(start, start + data_sz);
        }
    }

    // Add core dump header size
    data_len += sizeof(core_dump_header_t);
		//Checksum
		uint32_t cs_len = esp_core_dump_checksum_size();
		data_len += cs_len;

		return data_len;
}

static bool esp_core_dump_write_binary_port(sTcsCoredumpWriteCtx *write_cfg)
{
    esp_err_t err = ESP_OK;
    uint32_t tcb_sz = esp_core_dump_get_tcb_len();
    uint32_t data_len = 0;
    uint32_t bad_tasks_num = 0;
    core_dump_header_t hdr = { 0 };
    core_dump_task_header_t task_hdr = { 0 };
    core_dump_mem_seg_header_t mem_seg = { 0 };
    void *task = NULL;
    void *cur_task = NULL;

		// Verifies all tasks in the snapshot
    esp_core_dump_reset_tasks_snapshots_iter();
    while ((task = esp_core_dump_get_next_task(task))) {
        if (!esp_core_dump_get_task_snapshot(task, &task_hdr, &mem_seg)) {
            bad_tasks_num++;
            continue;
        }
        hdr.tasks_num++;
        if (task == esp_core_dump_get_current_task_handle()) {
            cur_task = task;
            ESP_COREDUMP_LOG_PROCESS("Task %x %x is first crashed task.", cur_task, task_hdr.tcb_addr);
        }
        ESP_COREDUMP_LOG_PROCESS("Stack len = %lu (%x %x)", task_hdr.stack_end-task_hdr.stack_start,
                                    task_hdr.stack_start, task_hdr.stack_end);
        // Increase core dump size by task stack size
        uint32_t stk_vaddr = 0;
        uint32_t stk_paddr = 0;
        uint32_t stk_len = esp_core_dump_get_stack(&task_hdr, &stk_vaddr, &stk_paddr);
        data_len += esp_core_dump_get_memory_len(stk_vaddr, stk_vaddr+stk_len);
        // Add tcb size
        data_len += (tcb_sz + sizeof(core_dump_task_header_t));
        if (mem_seg.size > 0) {
            ESP_COREDUMP_LOG_PROCESS("Add interrupted task stack %lu bytes @ %x",
                    mem_seg.size, mem_seg.start);
            data_len += esp_core_dump_get_memory_len(mem_seg.start, mem_seg.start+mem_seg.size);
            data_len += sizeof(core_dump_mem_seg_header_t);
            hdr.mem_segs_num++;
        }
    }
    ESP_COREDUMP_LOGI("First round Found tasks: good %d, bad %d, mem segs %d", hdr.tasks_num, bad_tasks_num, hdr.mem_segs_num);

    // Check if current task TCB is broken
    if (cur_task == NULL) {
        ESP_COREDUMP_LOG_PROCESS("The current crashed task is broken.");
        cur_task = esp_core_dump_get_next_task(NULL);
        if (cur_task == NULL) {
            ESP_COREDUMP_LOGE("No valid tasks in the system!");
            return false;
        }
    }

    // Add user memory regions data size
    for (coredump_region_t i = COREDUMP_MEMORY_START; i < COREDUMP_MEMORY_MAX; i++) {
        uint32_t start = 0;
        int data_sz = esp_core_dump_get_user_ram_info(i, &start);
        if (data_sz < 0) {
            ESP_COREDUMP_LOGE("Invalid memory segment size!");
            return false;
        }
        if (data_sz > 0) {
            hdr.mem_segs_num++;
            data_len += sizeof(core_dump_mem_seg_header_t) + esp_core_dump_get_memory_len(start, start + data_sz);
        }
    }
		
    // Add core dump header size
    data_len += sizeof(core_dump_header_t);

		wr_cfg.priv = &wr_data;
		memset(&wr_data, 0, sizeof(core_dump_write_data_t));

		//Checksum
		uint32_t cs_len = esp_core_dump_checksum_size();
		data_len += cs_len;

		if(write_cfg->compute_size_only){
				write_cfg->offset += data_len;
				return true;
		}

    ESP_COREDUMP_LOG_PROCESS("Core dump length=%lu, tasks processed: %d, broken tasks: %d",
                                data_len, hdr.tasks_num, bad_tasks_num);
    // Init checksum
    esp_core_dump_checksum_init(&wr_data.checksum_ctx);

    // Write header
    hdr.data_len  = data_len;
    hdr.version   = COREDUMP_VERSION_BIN_CURRENT;
    hdr.tcb_sz    = tcb_sz;
		proxy_platform_coredump_write(&hdr, sizeof(core_dump_header_t), write_cfg);

    // Save tasks
    esp_core_dump_reset_tasks_snapshots_iter();
    // Write first crashed task data first (not always first task in the snapshot)
    ESP_COREDUMP_LOGD("Save first crashed task %x", cur_task);
    if (esp_core_dump_get_task_snapshot(cur_task, &task_hdr, NULL)) {
        err = esp_core_dump_save_task(write_cfg, &task_hdr);
        if (err != ESP_OK) {
            ESP_COREDUMP_LOGE("Failed to save first crashed task %x, error=%d!",
                                task_hdr.tcb_addr, err);
            return err;
        }
    }
    // Write all other tasks in the snapshot
    task = NULL;
    while ((task = esp_core_dump_get_next_task(task))) {
        if (!esp_core_dump_get_task_snapshot(task, &task_hdr, NULL))
            continue;
        // Skip first crashed task
        if (task == cur_task) {
            continue;
        }
        ESP_COREDUMP_LOGD("Save task %x (TCB:%x, stack:%x..%x)", task, task_hdr.tcb_addr, task_hdr.stack_start, task_hdr.stack_end);
        err = esp_core_dump_save_task(write_cfg, &task_hdr);
        if (err != ESP_OK) {
            ESP_COREDUMP_LOGE("Failed to save core dump task %x, error=%d!",
                                    task_hdr.tcb_addr, err);
            return err;
        }
    }

    // Save interrupted stacks of the tasks
    // Actually there can be tasks interrupted at the same time, one on every core including the crashed one.
    task = NULL;
    esp_core_dump_reset_tasks_snapshots_iter();
    while ((task = esp_core_dump_get_next_task(task))) {
        if (!esp_core_dump_get_task_snapshot(task, &task_hdr, &mem_seg))
            continue;
        if (mem_seg.size > 0) {
            ESP_COREDUMP_LOG_PROCESS("Save interrupted task stack %lu bytes @ %x",
                    mem_seg.size, mem_seg.start);
            err = esp_core_dump_save_mem_segment(write_cfg, &mem_seg);
            if (err != ESP_OK) {
                ESP_COREDUMP_LOGE("Failed to save interrupted task stack, error=%d!", err);
                return err;
            }
        }
    }

    // save user memory regions
    if (esp_core_dump_get_user_ram_segments() > 0) {
        for (coredump_region_t i = COREDUMP_MEMORY_START; i < COREDUMP_MEMORY_MAX; i++) {
            uint32_t start = 0;
            int data_sz = esp_core_dump_get_user_ram_info(i, &start);

            if (data_sz < 0) {
                ESP_COREDUMP_LOGE("Invalid memory segment size");
                return ESP_FAIL;
            }

            if (data_sz > 0) {
                mem_seg.start = start;
                mem_seg.size = esp_core_dump_get_memory_len(start, start + data_sz);;
                ESP_COREDUMP_LOG_PROCESS("Save user memory region %lu bytes @ %x",
                        mem_seg.size, mem_seg.start);
                err = esp_core_dump_save_mem_segment(write_cfg, &mem_seg);
                if (err != ESP_OK) {
                    ESP_COREDUMP_LOGE("Failed to save user memory region, error=%d!", err);
                    return err;
                }
            }
        }
    }

    // Write end
		core_dump_checksum_bytes checksum = NULL;
    esp_core_dump_checksum_finish(wr_data.checksum_ctx, &checksum);
		prv_platform_coredump_write(checksum, cs_len, write_cfg);

    if (bad_tasks_num) {
        ESP_COREDUMP_LOGE("Found %d broken tasks!", bad_tasks_num);
    }
    return err;
}

bool platform_coredump_write_bin(sTcsCoredumpWriteCtx *write_ctx) {

	esp_core_dump_setup_stack();
	esp_core_dump_write_binary_port(write_ctx);
	esp_core_dump_report_stack_usage();

  return true;
}