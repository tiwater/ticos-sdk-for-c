//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port of dependency functions for Ticos core subsystem using FreeRTOS.

#include "ticos/ports/freertos.h"

#include "ticos/core/platform/overrides.h"
#include "ticos/core/platform/core.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#if configUSE_MUTEXES == 0
#error "configUSE_MUTEXES must be enabled to use Ticos FreeRTOS port"
#endif

#if configUSE_RECURSIVE_MUTEXES == 0
#error "configUSE_RECURSIVE_MUTEXES must be enabled to use Ticos FreeRTOS port"
#endif

uint64_t ticos_platform_get_time_since_boot_ms(void) {
  static uint64_t s_elapsed_ticks = 0;
  static uint32_t s_last_tick_count = 0;

  taskENTER_CRITICAL();
  {
    const uint32_t curr_tick_count = xTaskGetTickCount();

    // NB: Since we are doing unsigned arithmetic here, this operation will still work when
    // xTaskGetTickCount() has overflowed and wrapped around
    s_elapsed_ticks += (curr_tick_count - s_last_tick_count);
    s_last_tick_count = curr_tick_count;
  }
  taskEXIT_CRITICAL();

  return (s_elapsed_ticks * 1000) / configTICK_RATE_HZ;
}

static SemaphoreHandle_t s_ticos_lock;

SemaphoreHandle_t prv_init_ticos_mutex(void) {
#if TICOS_FREERTOS_PORT_USE_STATIC_ALLOCATION != 0
  static StaticSemaphore_t s_ticos_lock_context;
  return xSemaphoreCreateRecursiveMutexStatic(&s_ticos_lock_context);
#else
  return xSemaphoreCreateRecursiveMutex();
#endif
}

void ticos_lock(void) {
  xSemaphoreTakeRecursive(s_ticos_lock, portMAX_DELAY);
}

void ticos_unlock(void) {
  xSemaphoreGiveRecursive(s_ticos_lock);
}

void ticos_freertos_port_boot(void) {
  s_ticos_lock = prv_init_ticos_mutex();
}
