//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Logic for saving a coredump to backing storage and reading it out

#include "ticos/panics/coredump.h"
#include "ticos/panics/coredump_impl.h"

#include <string.h>
#include <stdbool.h>

#include "esp_private/panic_internal.h"

#include "ticos/core/build_info.h"
#include "ticos/core/compiler.h"
#include "ticos/core/data_packetizer_source.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/panics/platform/coredump.h"

#define TICOS_COREDUMP_MAGIC 0x45524f43

//! Version 2
//!  - If there is not enough storage space for memory regions,
//!    coredumps will now be truncated instead of failing completely
//!  - Added sTcsCoredumpFooter to end of coredump. In this region
//!    we track whether or not a coredump was truncated.
#define TICOS_COREDUMP_VERSION 2

typedef TICOS_PACKED_STRUCT TcsCoredumpHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t total_size;
  uint8_t data[];
} sTcsCoredumpHeader;

#define TICOS_COREDUMP_FOOTER_MAGIC 0x504d5544

typedef enum TcsCoredumpFooterFlags  {
  kTcsCoredumpBlockType_SaveTruncated = 0,
} eTcsCoredumpFooterFlags;

typedef TICOS_PACKED_STRUCT TcsCoredumpFooter {
  uint32_t magic;
  uint32_t flags;
  // reserving for future footer additions such as a CRC over the contents saved
  uint32_t rsvd[2];
} sTcsCoredumpFooter;

typedef TICOS_PACKED_STRUCT TcsCoredumpBlock {
  eTcsCoredumpBlockType block_type:8;
  uint8_t rsvd[3];
  uint32_t address;
  uint32_t length;
} sTcsCoredumpBlock;

typedef TICOS_PACKED_STRUCT TcsTraceReasonBlock {
  uint32_t reason;
} sTcsTraceReasonBlock;

// Using ELF machine enum values which is a half word:
//  https://refspecs.linuxfoundation.org/elf/gabi4%2B/ch4.eheader.html
//
// NB: We use the upper 16 bits of the MachineType TLV pair in the coredump to
// encode additional metadata about the architecture being targeted

#define TICOS_MACHINE_TYPE_SUBTYPE_OFFSET 16

#define TICOS_MACHINE_TYPE_XTENSA 94

#define TICOS_MACHINE_TYPE_XTENSA_LX106 \
  ((1 << TICOS_MACHINE_TYPE_SUBTYPE_OFFSET) | TICOS_MACHINE_TYPE_XTENSA)

typedef enum TcsCoredumpMachineType  {
  kTcsCoredumpMachineType_None = 0,
  kTcsCoredumpMachineType_ARM = 40,
  kTcsCoredumpMachineType_Aarch64 = 183,
  kTcsCoredumpMachineType_Xtensa = TICOS_MACHINE_TYPE_XTENSA,
  kTcsCoredumpMachineType_XtensaLx106 = TICOS_MACHINE_TYPE_XTENSA_LX106
} eTcsCoredumpMachineType;

typedef TICOS_PACKED_STRUCT TcsMachineTypeBlock {
  uint32_t machine_type;
} sTcsMachineTypeBlock;

// Checks to see if the block is a cached region and applies
// required fixups to allow the coredump to properly record
// the original cached address and its associated data. Will
// succeed if not a cached block or is a valid cached block.
// Callers should ignore the region if failure is returned
// because the block is not valid.
static bool prv_fixup_if_cached_block(sTcsCoredumpRegion *region, uint32_t *cached_address) {

  if (region->type ==  kTcsCoredumpRegionType_CachedMemory) {
    const sTcsCachedBlock *cached_blk = region->region_start;
    if (!cached_blk->valid_cache) {
      // Ignore this block.
      return false;
    }

    // This is where we want Ticos to indicate the data came from.
    *cached_address = cached_blk->cached_address;

    // The cached block is just regular memory.
    region->type = kTcsCoredumpRegionType_Memory;

    // Remove our header from the size and region_start
    // is where we cached the <cached_address>'s data.
    region->region_size = cached_blk->blk_size;
    region->region_start = cached_blk->blk; // Must be last operation!
  }

  // Success, untouched or fixed up.
  return true;
}

static bool prv_platform_coredump_write(const void *data, size_t len, sTcsCoredumpWriteCtx *write_ctx) {
  // if we are just computing the size needed, don't write any data but keep
  // a count of how many bytes would be written.
  if (!write_ctx->compute_size_only &&
      !ticos_platform_coredump_storage_write(write_ctx->offset, data, len)) {
    write_ctx->write_error = true;
    return false;
  }

  write_ctx->offset += len;
  return true;
}

static eTcsCoredumpBlockType prv_region_type_to_storage_type(eTcsCoredumpRegionType type) {
  switch (type) {
    case kTcsCoredumpRegionType_ArmV6orV7MpuUnrolled:
      return kTcsCoredumpRegionType_ArmV6orV7Mpu;
    case kTcsCoredumpRegionType_ImageIdentifier:
    case kTcsCoredumpRegionType_Memory:
    case kTcsCoredumpRegionType_MemoryWordAccessOnly:
    case kTcsCoredumpRegionType_CachedMemory:
    default:
      return kTcsCoredumpBlockType_MemoryRegion;
  }
}

extern panic_info_t *global_panic_info;

static bool prv_write_block_with_address(
    eTcsCoredumpBlockType block_type, const void *block_payload, size_t block_payload_size,
    uint32_t address, sTcsCoredumpWriteCtx *write_ctx, bool word_aligned_reads_only) {
  // nothing to write, ignore the request
  if (block_payload_size == 0 || (block_payload == NULL)) {
    return true;
  }

  const size_t total_length = sizeof(sTcsCoredumpBlock) + block_payload_size;
  const size_t storage_bytes_free =
      write_ctx->storage_size > write_ctx->offset ?  write_ctx->storage_size - write_ctx->offset : 0;

  if (!write_ctx->compute_size_only && storage_bytes_free < total_length) {
    // We are trying to write a new block in the coredump and there is not enough
    // space. Let's see if we can truncate the block to fit in the space that is left
    write_ctx->truncated = true;

    if (storage_bytes_free < sizeof(sTcsCoredumpBlock)) {
      return false;
    }

    block_payload_size = TICOS_FLOOR(storage_bytes_free - sizeof(sTcsCoredumpBlock), 4);
    if (block_payload_size == 0) {
      return false;
    }
  }

#if	defined(CONFIG_IDF_TARGET_ESP32)||defined(CONFIG_IDF_TARGET_ESP32S2)||defined(CONFIG_IDF_TARGET_ESP32S3)
	if(global_panic_info != NULL && block_type == prv_region_type_to_storage_type(kTcsCoredumpRegionType_Memory) &&
		!write_ctx->compute_size_only){
			// If it's memory region, write header later for the size calculation
			uint32_t hdr_offset = write_ctx->offset;
			write_ctx->offset += sizeof(sTcsCoredumpBlock);
			uint32_t region_offset = write_ctx->offset;
			platform_write_coredump_region(write_ctx);
			
			block_payload_size = write_ctx->offset - region_offset;

			const sTcsCoredumpBlock blk = {
				.block_type = block_type,
				.address = address,
				.length = block_payload_size,
			};

			write_ctx->offset = hdr_offset;

			if (!prv_platform_coredump_write(&blk, sizeof(blk), write_ctx)) {
				return false;
			}
			write_ctx->offset += block_payload_size;
			return true;
	}
#endif

  const sTcsCoredumpBlock blk = {
    .block_type = block_type,
    .address = address,
    .length = block_payload_size,
  };

  if (!prv_platform_coredump_write(&blk, sizeof(blk), write_ctx)) {
    return false;
  }

  if (!word_aligned_reads_only || ((block_payload_size % 4) != 0)) {
    // no requirements on how the 'address' is read so whatever the user implementation does is fine
    return prv_platform_coredump_write(block_payload, block_payload_size, write_ctx);
  }

  // We have a region that needs to be read 32 bits at a time.
  //
  // Typically these are very small regions such as a memory mapped register address
  const uint32_t *word_data = block_payload;
  for (uint32_t i = 0; i < block_payload_size / 4; i++) {
    const uint32_t data = word_data[i];
    if (!prv_platform_coredump_write(&data, sizeof(data), write_ctx)) {
      return false;
    }
  }

  return !write_ctx->truncated;
}

static bool prv_write_non_memory_block(eTcsCoredumpBlockType block_type,
                                       const void *block_payload, size_t block_payload_size,
                                       sTcsCoredumpWriteCtx *ctx) {
  const bool word_aligned_reads_only = false;
  return prv_write_block_with_address(block_type, block_payload, block_payload_size,
                                      0, ctx, word_aligned_reads_only);
}

static eTcsCoredumpMachineType prv_get_machine_type(void) {
#if defined(TICOS_UNITTEST)
  return kTcsCoredumpMachineType_None;
#else
#  if TICOS_COMPILER_ARM
  return kTcsCoredumpMachineType_ARM;
#  elif defined(__aarch64__)
  return kTcsCoredumpMachineType_Aarch64;
#  elif defined(__XTENSA__)
  # if defined(__XTENSA_WINDOWED_ABI__)
    return kTcsCoredumpMachineType_Xtensa;
  # else
    return kTcsCoredumpMachineType_XtensaLx106;
  # endif
# else
#    error "Coredumps are not supported for target architecture"
#  endif
#endif
}

static bool prv_write_device_info_blocks(sTcsCoredumpWriteCtx *ctx) {
  struct TicosDeviceInfo info;
  ticos_platform_get_device_info(&info);

#if TICOS_COREDUMP_INCLUDE_BUILD_ID
  sTicosBuildInfo build_info;
  if (ticos_build_info_read(&build_info)) {
    if (!prv_write_non_memory_block(kTcsCoredumpRegionType_BuildId,
                                    build_info.build_id, sizeof(build_info.build_id), ctx)) {
      return false;
    }
  }
#endif

  if (info.device_serial) {
    if (!prv_write_non_memory_block(kTcsCoredumpRegionType_DeviceSerial,
                                    info.device_serial, strlen(info.device_serial), ctx)) {
      return false;
    }
  }

  if (info.software_version) {
    if (!prv_write_non_memory_block(kTcsCoredumpRegionType_SoftwareVersion,
                                    info.software_version, strlen(info.software_version), ctx)) {
      return false;
    }
  }

  if (info.software_type) {
    if (!prv_write_non_memory_block(kTcsCoredumpRegionType_SoftwareType,
                                       info.software_type, strlen(info.software_type), ctx)) {
      return false;
    }
  }

  if (info.hardware_version) {
    if (!prv_write_non_memory_block(kTcsCoredumpRegionType_HardwareVersion,
                                       info.hardware_version, strlen(info.hardware_version), ctx)) {
      return false;
    }
  }

  eTcsCoredumpMachineType machine_type = prv_get_machine_type();
  const sTcsMachineTypeBlock machine_block = {
    .machine_type = (uint32_t)machine_type,
  };
  return prv_write_non_memory_block(kTcsCoredumpRegionType_MachineType,
                                    &machine_block, sizeof(machine_block), ctx);
}

static bool prv_write_coredump_header(size_t total_coredump_size, sTcsCoredumpWriteCtx *ctx) {
  sTcsCoredumpHeader hdr = (sTcsCoredumpHeader) {
    .magic = TICOS_COREDUMP_MAGIC,
    .version = TICOS_COREDUMP_VERSION,
    .total_size = total_coredump_size,
  };
  return prv_platform_coredump_write(&hdr, sizeof(hdr), ctx);
}

static bool prv_write_trace_reason(sTcsCoredumpWriteCtx *ctx, uint32_t trace_reason) {
  sTcsTraceReasonBlock trace_info = {
    .reason = trace_reason,
  };

  return prv_write_non_memory_block(kTcsCoredumpRegionType_TraceReason,
                                    &trace_info, sizeof(trace_info), ctx);
}

// When copying out some regions (for example, memory or register banks)
// we want to make sure we can do word-aligned accesses.
static void prv_insert_padding_if_necessary(sTcsCoredumpWriteCtx *write_ctx) {
  #define TICOS_WORD_SIZE 4
  const size_t remainder = write_ctx->offset % TICOS_WORD_SIZE;
  if (remainder == 0) {
    return;
  }

  #define TICOS_MAX_PADDING_BYTES_NEEDED (TICOS_WORD_SIZE - 1)
  uint8_t pad_bytes[TICOS_MAX_PADDING_BYTES_NEEDED];

  size_t padding_needed = TICOS_WORD_SIZE - remainder;
  memset(pad_bytes, 0x0, padding_needed);

  prv_write_non_memory_block(kTcsCoredumpRegionType_PaddingRegion,
                             &pad_bytes, padding_needed, write_ctx);
}

//! Callback that will be called to write coredump data.
typedef bool(*TcsCoredumpReadCb)(uint32_t offset, void *data, size_t read_len);


static bool prv_get_info_and_header(sTcsCoredumpHeader *hdr_out,
                                    sTcsCoredumpStorageInfo *info_out,
                                    TcsCoredumpReadCb coredump_read_cb) {
  sTcsCoredumpStorageInfo info = { 0 };
  ticos_platform_coredump_storage_get_info(&info);
  if (info.size == 0) {
    return false; // no space for core files!
  }

  if (!coredump_read_cb(0, hdr_out, sizeof(*hdr_out))) {
    // NB: This path is sometimes _expected_. For situations where
    // ticos_platform_coredump_storage_clear() is an asynchronous operation a caller may return
    // false for from ticos_coredump_read() to prevent any access to the coredump storage area.
    return false;
  }

  if (info_out) {
    *info_out = info;
  }
  return true;
}

static bool prv_coredump_get_header(sTcsCoredumpHeader *hdr_out,
                                    TcsCoredumpReadCb coredump_read_cb) {
  return prv_get_info_and_header(hdr_out, NULL, coredump_read_cb);
}

static bool prv_coredump_header_is_valid(const sTcsCoredumpHeader *hdr) {
  return (hdr && hdr->magic == TICOS_COREDUMP_MAGIC);
}

static bool prv_write_regions(sTcsCoredumpWriteCtx *write_ctx, const sTcsCoredumpRegion *regions,
                              size_t num_regions) {
  for (size_t i = 0; i < num_regions; i++) {
    prv_insert_padding_if_necessary(write_ctx);

    // Just in case *regions is some how in r/o memory make a non-const copy
    // and work with that from here on.
    sTcsCoredumpRegion region_copy = regions[i];

    uint32_t address = (uint32_t)(uintptr_t)region_copy.region_start;
    if (!prv_fixup_if_cached_block(&region_copy, &address)) {
      // We must skip invalid cached blocks.
      continue;
    }

    const bool word_aligned_reads_only =
        (region_copy.type == kTcsCoredumpRegionType_MemoryWordAccessOnly);

    if (!prv_write_block_with_address(prv_region_type_to_storage_type(region_copy.type),
                                      region_copy.region_start, region_copy.region_size,
                                      address, write_ctx, word_aligned_reads_only)) {
      return false;
    }
  }
  return true;
}

static bool prv_write_coredump_sections(const sTicosCoredumpSaveInfo *save_info,
                                        bool compute_size_only, size_t *total_size) {
  sTcsCoredumpStorageInfo info = { 0 };
  sTcsCoredumpHeader hdr = { 0 };

  // are there some regions for us to save?
  size_t num_regions = save_info->num_regions;
  const sTcsCoredumpRegion *regions = save_info->regions;
  if ((regions == NULL) || (num_regions == 0)) {
    // sanity check that we got something valid from the caller
    return false;
  }

  if (!compute_size_only) {
    if (!ticos_platform_coredump_save_begin()) {
      return false;
    }

    // If we are saving a new coredump but one is already stored, don't overwrite it. This way
    // the first issue which started the crash loop can be determined
    TcsCoredumpReadCb coredump_read_cb = ticos_platform_coredump_storage_read;
    if (!prv_get_info_and_header(&hdr, &info, coredump_read_cb)) {
      return false;
    }

    if (prv_coredump_header_is_valid(&hdr)) {
      return false; // don't overwrite what we got!
    }
  }

  // erase storage provided we aren't just computing the size
  if (!compute_size_only && !ticos_platform_coredump_storage_erase(0, info.size)) {
    return false;
  }

  sTcsCoredumpWriteCtx write_ctx = {
    // We will write the header last as a way to mark validity
    // so advance the offset past it to start
    .offset = sizeof(hdr),
    .compute_size_only = compute_size_only,
    .storage_size = info.size,
  };

  if (write_ctx.storage_size > sizeof(sTcsCoredumpFooter)) {
    // always leave space for footer
    write_ctx.storage_size -= sizeof(sTcsCoredumpFooter);
  }

  const void *regs = save_info->regs;
  const size_t regs_size = save_info->regs_size;
  if (regs != NULL) {
    if (!prv_write_non_memory_block(kTcsCoredumpBlockType_CurrentRegisters,
                                    regs, regs_size, &write_ctx)) {
      return false;
    }
  }

  if (!prv_write_device_info_blocks(&write_ctx)) {
    return false;
  }

  const uint32_t trace_reason = save_info->trace_reason;
  if (!prv_write_trace_reason(&write_ctx, trace_reason)) {
    return false;
  }

  // write out any architecture specific regions
  size_t num_arch_regions = 0;
  const sTcsCoredumpRegion *arch_regions = ticos_coredump_get_arch_regions(&num_arch_regions);
  size_t num_sdk_regions = 0;
  const sTcsCoredumpRegion *sdk_regions = ticos_coredump_get_sdk_regions(&num_sdk_regions);

  const bool write_completed =
      prv_write_regions(&write_ctx, arch_regions, num_arch_regions) &&
      prv_write_regions(&write_ctx, sdk_regions, num_sdk_regions) &&
      prv_write_regions(&write_ctx, regions, num_regions);

  if (!write_completed && write_ctx.write_error) {
    return false;
  }

  const sTcsCoredumpFooter footer = (sTcsCoredumpFooter) {
    .magic = TICOS_COREDUMP_FOOTER_MAGIC,
    .flags = write_ctx.truncated ? (1 << kTcsCoredumpBlockType_SaveTruncated) : 0,
  };
  write_ctx.storage_size = info.size;
  if (!prv_platform_coredump_write(&footer, sizeof(footer), &write_ctx)) {
    return false;
  }

  const size_t end_offset = write_ctx.offset;
  write_ctx.offset = 0; // we are writing the header so reset our write offset
  const bool success = prv_write_coredump_header(end_offset, &write_ctx);
  if (success) {
    *total_size = end_offset;
  }

  return success;
}

TICOS_WEAK
bool ticos_platform_coredump_save_begin(void) {
  return true;
}

size_t ticos_coredump_get_save_size(const sTicosCoredumpSaveInfo *save_info) {
  const bool compute_size_only = true;
  size_t total_size = 0;
  prv_write_coredump_sections(save_info, compute_size_only, &total_size);
  return total_size;
}

bool ticos_coredump_save(const sTicosCoredumpSaveInfo *save_info) {
  const bool compute_size_only = false;
  size_t total_size = 0;
  return prv_write_coredump_sections(save_info, compute_size_only, &total_size);
}

bool ticos_coredump_has_valid_coredump(size_t *total_size_out) {
  sTcsCoredumpHeader hdr = { 0 };
  // This routine is only called while the system is running so _always_ use the
  // ticos_coredump_read, which is safe to call while the system is running
  TcsCoredumpReadCb coredump_read_cb = ticos_coredump_read;
  if (!prv_coredump_get_header(&hdr, coredump_read_cb)) {
    return false;
  }
  if (!prv_coredump_header_is_valid(&hdr)) {
    return false;
  }
  if (total_size_out) {
    *total_size_out = hdr.total_size;
  }
  return true;
}

TICOS_WEAK
bool ticos_coredump_read(uint32_t offset, void *buf, size_t buf_len) {
  return ticos_platform_coredump_storage_read(offset, buf, buf_len);
}

//! Expose a data source for use by the Ticos Packetizer
const sTicosDataSourceImpl g_ticos_coredump_data_source = {
  .has_more_msgs_cb = ticos_coredump_has_valid_coredump,
  .read_msg_cb = ticos_coredump_read,
  .mark_msg_read_cb = ticos_platform_coredump_storage_clear,
};
