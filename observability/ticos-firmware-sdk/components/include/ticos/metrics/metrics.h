#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! The Ticos metric events API
//!
//! This APIs allows one to collect periodic events known as heartbeats for visualization in the
//! Ticos web UI. Heartbeats are a great way to inspect the overall health of devices in your
//! fleet.
//!
//! Typically, two types of information are collected:
//! 1) values taken at the end of the interval (i.e battery life, heap high water mark, stack high
//! water mark)
//! 2) changes over the hour (i.e the percent battery drop, the number of bytes sent out over
//!    bluetooth, the time the mcu was running or in stop mode)
//!
//! From the Ticos web UI, you can view all of these metrics plotted for an individual device &
//! configure alerts to fire when values are not within an expected range.
//!
//! For a step-by-step walk-through about how to integrate the Metrics component into your system,
//! check out https://ticos.io/2D8TRLX
//!
//! For more details of the overall design and how serialization compression works check out:
//!  https://ticos.io/fw-event-serialization

#include <inttypes.h>

#include "ticos/config.h"
#include "ticos/core/event_storage.h"
#include "ticos/metrics/ids_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "ticos/core/compiler.h"

//! Type of a metric value
typedef enum TicosMetricValueType {
  //! unsigned integer (max. 32-bits)
  kTicosMetricType_Unsigned = 0,
  //! signed integer (max. 32-bits)
  kTicosMetricType_Signed,
  //! Tracks durations (i.e the time a certain task is running, or the time a MCU is in sleep mode)
  kTicosMetricType_Timer,
  //! Set a string value for a metric
  kTicosMetricType_String,

  //! Number of valid types. Must _always_ be last
  kTicosMetricType_NumTypes,
} eTicosMetricType;

//! Defines a key/value pair used for generating Ticos events.
//!
//! This define should _only_ be used for defining events in 'ticos_metric_heartbeat_config.def'
//! i.e, the *.def file for a heartbeat which tracks battery level & temperature would look
//! something like:
//!
//! // ticos_metrics_heartbeat_config.def
//! TICOS_METRICS_KEY_DEFINE_WITH_RANGE(battery_level, kTicosMetricType_Unsigned, 0, 100)
//! TICOS_METRICS_KEY_DEFINE_WITH_RANGE(ambient_temperature_celcius, kTicosMetricType_Signed, -40, 40)
//!
//! @param key_name The name of the key, without quotes. This gets surfaced in the Ticos UI, so
//! it's useful to make these names human readable. C variable naming rules apply.
//! @param value_type The eTicosMetricType type of the key
//! @param min_value A hint as to what the expected minimum value of the metric will be
//! @param max_value A hint as to what the expected maximum value of the metric will be
//!
//! @note min_value & max_value are used to define an expected range for a given metric.
//! This information is used in the Ticos cloud to normalize the data to a range of your choosing.
//! Metrics will still be ingested _even_ if they are outside the range defined.
//!
//! @note The definitions here are used to catch accidental usage outside of the
//! '*_heartbeat_config.def' files; 'TICOS_METRICS_KEY_DEFINE_TRAP_' is a placeholder used to
//! detect
//! this.
//!
//! @note key_names must be unique
#define TICOS_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value) \
  TICOS_METRICS_KEY_DEFINE_TRAP_()

//! Same as 'TICOS_METRICS_KEY_DEFINE_WITH_RANGE' just with no range hints specified
#define TICOS_METRICS_KEY_DEFINE(key_name, value_type) \
  TICOS_METRICS_KEY_DEFINE_TRAP_()

//! Declare a string metric for use in a heartbeat. 'max_length' is the maximum
//! length of the string recorded to the metric (excluding null terminator).
//! This declaration also reserves space to hold the string value when the
//! metric is written.
#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  TICOS_METRICS_KEY_DEFINE_TRAP_()

//! Uses a metric key. Before you can use a key, it should defined using TICOS_METRICS_KEY_DEFINE
//! in ticos_metrics_heartbeat_config.def.
//! @param key_name The name of the key, without quotes, as defined using TICOS_METRICS_KEY_DEFINE.
#define TICOS_METRICS_KEY(key_name) \
  _TICOS_METRICS_ID(key_name)

typedef struct TicosMetricsBootInfo {
  //! The number of times the system has rebooted unexpectedly since reporting the last heartbeat
  //!
  //! If you do not already have a system in place to track this, consider using the ticos
  //! reboot_tracking module (https://ticos.io/2QlOlgH). This info can be collected by
  //! passing the value returned from ticos_reboot_tracking_get_crash_count().
  //! When using this API we recommend clearing the crash when the first heartbeat since boot
  //! is serialized by implementing ticos_metrics_heartbeat_collect_data() and calling
  //! ticos_reboot_tracking_reset_crash_count().
  //!
  //! If any reboot is unexpected, initialization can also reduce to:
  //!   sTicosMetricBootInfo info = { .unexpected_reboot_count = 1 };
  uint32_t unexpected_reboot_count;
} sTicosMetricBootInfo;

//! Initializes the metric events API.
//! All heartbeat values will be initialized to 0.
//! @param storage_impl The storage location to serialize metrics out to
//! @param boot_info Info added to metrics to facilitate computing aggregate statistics in
//!  the Ticos cloud
//! @note Ticos will start collecting metrics once this function returns.
//! @return 0 on success, else error code
int ticos_metrics_boot(const sTicosEventStorageImpl *storage_impl,
                          const sTicosMetricBootInfo *boot_info);

//! Set the value of a signed integer metric.
//! @param key The key of the metric. @see TICOS_METRICS_KEY
//! @param value The new value to set for the metric
//! @return 0 on success, else error code
//! @note The metric must be of type kTicosMetricType_Signed
int ticos_metrics_heartbeat_set_signed(TicosMetricId key, int32_t signed_value);

//! Same as @ticos_metrics_heartbeat_set_signed except for a unsigned integer metric
int ticos_metrics_heartbeat_set_unsigned(TicosMetricId key, uint32_t unsigned_value);

//! Set the value of a string metric.
//! @param key The key of the metric. @see TICOS_METRICS_KEY
//! @param value The new value to set for the metric
//! @return 0 on success, else error code
//! @note The metric must be of type kTicosMetricType_String
int ticos_metrics_heartbeat_set_string(TicosMetricId key, const char * value);

//! Used to start a "timer" metric
//!
//! Timer metrics can be useful for tracking durations of events which take place while the
//! system is running. Some examples:
//!  - time a task was running
//!  - time spent in different power modes (i.e run, sleep, idle)
//!  - amount of time certain peripherals were running (i.e accel, bluetooth, wifi)
//!
//! @return 0 if starting the metric was successful, else error code.
int ticos_metrics_heartbeat_timer_start(TicosMetricId key);

//! Same as @ticos_metrics_heartbeat_start but *stops* the timer metric
//!
//! @return 0 if stopping the timer was successful, else error code
int ticos_metrics_heartbeat_timer_stop(TicosMetricId key);

//! Add the value to the current value of a metric.
//! @param key The key of the metric. @see TICOS_METRICS_KEY
//! @param inc The amount to increment the metric by
//! @return 0 on success, else error code
//! @note The metric must be of type kTicosMetricType_Counter
int ticos_metrics_heartbeat_add(TicosMetricId key, int32_t amount);

//! For debugging purposes: prints the current heartbeat values using
//! TICOS_LOG_DEBUG(). Before printing, any active timer values are computed.
//! Other metrics will print the current values. This can be called from the
//! user-supplied ticos_metrics_heartbeat_collect_data() function to print
//! values set there (use ticos_metrics_heartbeat_debug_trigger() to trigger
//! a metrics collection).
void ticos_metrics_heartbeat_debug_print(void);

//! For debugging purposes: triggers the heartbeat data collection handler, as if the heartbeat timer
//! had fired. We recommend also testing that the heartbeat timer fires by itself. To get the periodic data
//! collection triggering rapidly for testing and debugging, consider using a small value for
//! TICOS_METRICS_HEARTBEAT_INTERVAL_SECS.
void ticos_metrics_heartbeat_debug_trigger(void);

//! For debugging and unit test purposes, allows for the extraction of different values
int ticos_metrics_heartbeat_read_unsigned(TicosMetricId key, uint32_t *read_val);
int ticos_metrics_heartbeat_read_signed(TicosMetricId key, int32_t *read_val);
int ticos_metrics_heartbeat_timer_read(TicosMetricId key, uint32_t *read_val);
int ticos_metrics_heartbeat_read_string(TicosMetricId key, char *read_val, size_t read_val_len);

//! Collect built-in metrics as part of default ports in ticos-firmware-sdk.
//!
//! It can (optionally) be overridden by a port to collect a set of built-in metrics
//! as configured by the port/application.
//!
//! @note By default, a weak version of this function is implemented which is empty
//! @note This API is for internal use only and should never be called by an end user
void ticos_metrics_heartbeat_collect_sdk_data(void);

#ifdef __cplusplus
}
#endif
