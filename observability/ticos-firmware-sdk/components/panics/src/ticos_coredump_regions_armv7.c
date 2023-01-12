//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Architecture-specific registers collected collected by Ticos SDK Extra decoding and analysis
//! of these registers is provided from the Ticos cloud

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"

#if TICOS_COMPILER_ARM

#include "ticos/core/math.h"
#include "ticos/panics/coredump.h"
#include "ticos/panics/coredump_impl.h"
#include "ticos/panics/platform/coredump.h"

TICOS_STATIC_ASSERT(((TICOS_NVIC_INTERRUPTS_TO_COLLECT) % 32 == 0)
                       || ((TICOS_NVIC_INTERRUPTS_TO_COLLECT) == 496),
                       "Must be a multiple of 32 or 496");
TICOS_STATIC_ASSERT((TICOS_NVIC_INTERRUPTS_TO_COLLECT) <= 512, "Exceeded max possible size");

TICOS_STATIC_ASSERT((TICOS_MPU_REGIONS_TO_COLLECT) <= 16, "Exceeded max possible size");

#if TICOS_COLLECT_FAULT_REGS
// Subset of ARMv7-M "System Control and ID blocks" related to fault status
// On some ports some of these registers may need to be pre-cached before
// the OS consumes them. For simplicity if any need pre-caching then we
// grab them all.
typedef TICOS_PACKED_STRUCT {
  uint32_t SHCSR;
  uint32_t CFSR;
  uint32_t HFSR;
  uint32_t DFSR;
  uint32_t MMFAR;
  uint32_t BFAR;
  uint32_t AFSR;
} sTcsFaultRegs;

#endif /* TICOS_COLLECT_FAULT_REGS */

typedef TICOS_PACKED_STRUCT {
  uint32_t ICTR;
  uint32_t ACTLR;
  uint32_t rsvd;
  uint32_t SYST_CSR;
} sTcsControlRegs;

typedef TICOS_PACKED_STRUCT {
  uint32_t ICSR;
  uint32_t VTOR;
} sTcsIntControlRegs;

typedef TICOS_PACKED_STRUCT {
  uint32_t SHPR1;
  uint32_t SHPR2;
  uint32_t SHPR3;
} sTcsSysHandlerPriorityRegs;

typedef TICOS_PACKED_STRUCT {
  uint32_t DEMCR;
} sTcsDebugExcMonCtrlReg;

typedef TICOS_PACKED_STRUCT {
  // representation for NVIC ISER, ISPR, and IABR ...
  // A single bit encodes whether or not the interrupt is enabled, pending, active, respectively.
  uint32_t IxxR[(TICOS_NVIC_INTERRUPTS_TO_COLLECT + 31) / 32];
} sTcsNvicIserIsprIabr;

typedef TICOS_PACKED_STRUCT {
  // 8 bits are used to encode the priority so 4 interrupts are covered by each register
  // NB: unimplemented priority levels read back as 0
  uint32_t IPR[TICOS_NVIC_INTERRUPTS_TO_COLLECT / 4];
} sTcsNvicIpr;

#if TICOS_COLLECT_MPU_STATE
// Number of regions (RNR) is implementation defined so the caller
// can configure this precisely based on what their HW provides and
// how their software employs the MPU. The array of RBAR/RASR pair
// structs attempts to flatten out the HW's paged registers that are
// exposed via RNR. RNR (@0xE000ED98) is not included in the dump
// as it adds no diagnostic value.
typedef TICOS_PACKED_STRUCT {
  uint32_t TYPE; // 0xE000ED90
  uint32_t CTRL; // 0xE000ED94
  struct {
    uint32_t RBAR; // 0xE000ED9C, Indexed by RNR
    uint32_t RASR; // 0xE000EDA0, Indexed by RNR
  } pair[TICOS_MPU_REGIONS_TO_COLLECT];
} sTcsMpuRegs;
#endif /* TICOS_COLLECT_MPU_STATE */


#if !TICOS_COLLECT_FAULT_REGS
#define FAULT_REG_REGION_TYPE  kTcsCoredumpRegionType_CachedMemory
#define FAULT_REG_REGION_START (0)
#define FAULT_REG_REGION_SIZE  (0)
#elif TICOS_CACHE_FAULT_REGS
#define FAULT_REG_REGION_TYPE  kTcsCoredumpRegionType_CachedMemory
#define FAULT_REG_REGION_START ((void*)&s_cached_fault_regs)
#define FAULT_REG_REGION_SIZE  (sizeof(s_cached_fault_regs))

// Raw buffer for a sTcsCachedBlock.
static uint32_t s_cached_fault_regs[TICOS_CACHE_BLOCK_SIZE_WORDS(sizeof(sTcsFaultRegs))];

// This fault register harvester function allows us to get unadulterated copies
// of the ARM fault registers before they are modified (cleared) by an OS. We send
// this information in place of whatever may be in the HW register by the
// time we get to them.
void ticos_coredump_cache_fault_regs(void) {
  sTcsCachedBlock *fault_regs = (sTcsCachedBlock *)&s_cached_fault_regs[0];
  fault_regs->cached_address = 0xE000ED24; // SCB->SHCSR

  const volatile uint32_t * const hwreg =
      (uint32_t *)fault_regs->cached_address;
  sTcsFaultRegs * const scb = (sTcsFaultRegs *) fault_regs->blk;

  scb->SHCSR = hwreg[offsetof(sTcsFaultRegs, SHCSR) / sizeof(*hwreg)];
  scb->CFSR  = hwreg[offsetof(sTcsFaultRegs, CFSR)  / sizeof(*hwreg)];
  scb->HFSR  = hwreg[offsetof(sTcsFaultRegs, HFSR)  / sizeof(*hwreg)];
  scb->DFSR  = hwreg[offsetof(sTcsFaultRegs, DFSR)  / sizeof(*hwreg)];
  scb->MMFAR = hwreg[offsetof(sTcsFaultRegs, MMFAR) / sizeof(*hwreg)];
  scb->BFAR  = hwreg[offsetof(sTcsFaultRegs, BFAR)  / sizeof(*hwreg)];
  scb->AFSR  = hwreg[offsetof(sTcsFaultRegs, AFSR)  / sizeof(*hwreg)];
  fault_regs->blk_size = sizeof(sTcsFaultRegs);
  fault_regs->valid_cache = true;
}
#else
#define FAULT_REG_REGION_TYPE  kTcsCoredumpRegionType_MemoryWordAccessOnly
#define FAULT_REG_REGION_START 0xE000ED24 // Start at SHCSR
#define FAULT_REG_REGION_SIZE  (sizeof(sTcsFaultRegs))
#endif

const sTcsCoredumpRegion *ticos_coredump_get_arch_regions(size_t *num_regions) {
#if TICOS_COLLECT_MPU_STATE
  static sTcsMpuRegs s_tcs_mpu_regs;

  // We need to unroll the MPU registers from hardware into the representation
  // that is parsed by the Ticos cloud. Be sure this algorithm matches the
  // Python version in ticos_gdb.py.
  s_tcs_mpu_regs.TYPE = *(uint32_t volatile *) 0xE000ED90;
  if (s_tcs_mpu_regs.TYPE) {
    // MPU is implemented, get the region count but don't
    // exceed the caller's limit if smaller than the HW supports.
    size_t num_mpu_regions = (s_tcs_mpu_regs.TYPE >> 8) & 0xFF;
    num_mpu_regions = TICOS_MIN(num_mpu_regions, TICOS_ARRAY_SIZE(s_tcs_mpu_regs.pair));

    // Save CTRL but skip RNR as it has no debug value.
    s_tcs_mpu_regs.CTRL = *(uint32_t volatile *) 0xE000ED94;

    // Unroll the paged register pairs into our array representation by select-and-read.
    for (size_t region = 0; region < num_mpu_regions; ++region) {
      *(uint32_t volatile *) 0xE000ED98 = region;
      s_tcs_mpu_regs.pair[region].RBAR = *(uint32_t volatile *) 0xE000ED9C;
      s_tcs_mpu_regs.pair[region].RASR = *(uint32_t volatile *) 0xE000EDA0;
    }
  }
#endif /* TICOS_COLLECT_MPU_STATE */

  static const sTcsCoredumpRegion s_coredump_regions[] = {
    {
      .type = FAULT_REG_REGION_TYPE,
      .region_start = (void *)FAULT_REG_REGION_START,
      .region_size = FAULT_REG_REGION_SIZE
    },
#if TICOS_COLLECT_INTERRUPT_STATE
    {
      .type = kTcsCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000ED18,
      .region_size = sizeof(sTcsSysHandlerPriorityRegs)
    },
    {
      .type = kTcsCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E004,
      .region_size = sizeof(sTcsControlRegs)
    },
    {
      .type = kTcsCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000ED04,
      .region_size = sizeof(sTcsIntControlRegs)
    },
    {
      .type = kTcsCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000EDFC,
      .region_size = sizeof(sTcsDebugExcMonCtrlReg)
    },
    {
      .type = kTcsCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E100,
      .region_size = sizeof(sTcsNvicIserIsprIabr)
    },
    {
      .type = kTcsCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E200,
      .region_size = sizeof(sTcsNvicIserIsprIabr)
    },
    {
      .type = kTcsCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E300,
      .region_size = sizeof(sTcsNvicIserIsprIabr)
    },
    {
      .type = kTcsCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E400,
      .region_size = sizeof(sTcsNvicIpr)
    },
#endif /* TICOS_COLLECT_INTERRUPT_STATE */

#if TICOS_COLLECT_MPU_STATE
    {
      .type = kTcsCoredumpRegionType_ArmV6orV7MpuUnrolled,
      .region_start = (void *) &s_tcs_mpu_regs, // Cannot point at the hardware, needs processing
      .region_size = sizeof(s_tcs_mpu_regs)
    },
#endif /* TICOS_COLLECT_MPU_STATE */
  };

  *num_regions = TICOS_ARRAY_SIZE(s_coredump_regions);
  return &s_coredump_regions[0];
}

#endif /* TICOS_COMPILER_ARM */
