#include <string.h>
#include "ticos/panics/coredump.h"
#include "esp_partition.h"
#include "esp_core_dump_port.h"
#include "esp_core_dump_common.h"
#include "core_dump_checksum.h"
#include "esp_spi_flash.h"
#include "esp_flash_internal.h"
#include "esp_core_dump_types.h"
#include "esp_rom_crc.h"

const static DRAM_ATTR char TAG[] __attribute__((unused)) = "ticos_platform_dump_flash";

extern panic_info_t *global_panic_info;

void __real_esp_core_dump_to_flash(panic_info_t *info);

void __real_esp_core_dump_write(panic_info_t *info, core_dump_write_config_t *write_cfg);

static esp_core_dump_write_prepare_t prepare_func;
static uint32_t esp_core_dump_offset;
static core_dump_write_config_t *core_dump_write_cfg;

/**
 * @brief To support write esp's coredump into the sdk package, hack the prepare function. Now it won't erase the partition and will set the offset
 * 
 * @param priv 
 * @param data_len 
 * @return esp_err_t 
 */
static esp_err_t wrap_esp_core_dump_flash_write_prepare(core_dump_write_data_t *priv, uint32_t *data_len){

		core_dump_write_data_t *wr_data = (core_dump_write_data_t *)priv;
    esp_err_t err = ESP_OK;
    uint32_t cs_len = 0;

    /* Get the length, in bytes, of the checksum. */
    cs_len = esp_core_dump_checksum_size();

    /* At the end of the core dump file, a padding may be added, according to the
     * cache size. We must take that padding into account. */
    uint32_t padding = 0;
    const uint32_t modulo = *data_len % COREDUMP_CACHE_SIZE;
    if (modulo != 0) {
        /* The data length is not a multiple of the cache size,
         * so there will be a padding. */
        padding = COREDUMP_CACHE_SIZE - modulo;
    }

    /* We have enough space in the partition, add the padding and the checksum
     * in the core dump file calculation. */
    *data_len += padding + cs_len + esp_core_dump_offset;

    memset(wr_data, 0, sizeof(core_dump_write_data_t));

		// Recover soffset
		wr_data->off = esp_core_dump_offset;

		return err;
}

void platform_write_coredump_region(sTcsCoredumpWriteCtx *write_ctx){
		esp_core_dump_offset = write_ctx->offset;
		esp_core_dump_flash_init();

		// Call esp's func to dump the mem to flash
		__real_esp_core_dump_to_flash(global_panic_info);

		core_dump_write_data_t *wr_data = (core_dump_write_data_t *)core_dump_write_cfg->priv;
		write_ctx->offset = wr_data->off;
}

void __wrap_esp_core_dump_write(panic_info_t *info, core_dump_write_config_t *write_cfg){

		// Replace the prepare function of esp coredump functions
		prepare_func = write_cfg->prepare;
		write_cfg->prepare = wrap_esp_core_dump_flash_write_prepare;
		core_dump_write_cfg = write_cfg;
		__real_esp_core_dump_write(info, write_cfg);
}