#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Internal keys used for serializing different types of event ids. Users of the SDK should never
//! need to use any of these directly.

#ifdef __cplusplus
extern "C" {
#endif

#define TICOS_CBOR_SCHEMA_VERSION_V1 (1) // NOTE: implies "sdk_version": "0.5.0"

typedef enum {
  kTicosEventKey_CapturedDateUnixTimestamp = 1,
  kTicosEventKey_Type = 2,
  kTicosEventKey_CborSchemaVersion = 3,
  kTicosEventKey_EventInfo = 4,
  kTicosEventKey_UserInfo = 5,
  kTicosEventKey_HardwareVersion = 6,
  kTicosEventKey_DeviceSerial = 7,
  kTicosEventKey_ReleaseVersionDeprecated = 8,
  kTicosEventKey_SoftwareVersion = 9,
  kTicosEventKey_SoftwareType = 10,
  kTicosEventKey_BuildId = 11,
} eTicosEventKey;

//! Possible values for the kTicosEventKey_Type field.
typedef enum {
  kTicosEventType_Heartbeat = 1,
  kTicosEventType_Trace = 2,
  kTicosEventType_LogError = 3,
  kTicosEventType_Logs = 4,
  kTicosEventType_Cdr = 5,
} eTicosEventType;

//! EventInfo dictionary keys for events with type kTicosEventType_Heartbeat.
typedef enum {
  kTicosHeartbeatInfoKey_Metrics = 1,
} eTicosHeartbeatInfoKey;

//! EventInfo dictionary keys for events with type kTicosEventType_Trace.
typedef enum {
  kTicosTraceInfoEventKey_Reason = 1,
  kTicosTraceInfoEventKey_ProgramCounter = 2,
  kTicosTraceInfoEventKey_LinkRegister = 3,
  kTicosTraceInfoEventKey_McuReasonRegister = 4,
  kTicosTraceInfoEventKey_CoredumpSaved = 5,
  kTicosTraceInfoEventKey_UserReason = 6,
  kTicosTraceInfoEventKey_StatusCode = 7,
  kTicosTraceInfoEventKey_Log = 8,
  kTicosTraceInfoEventKey_CompactLog = 9,
} eTicosTraceInfoEventKey;

//! EventInfo dictionary keys for events with type kTicosEventType_LogError.
typedef enum {
  kTicosLogErrorInfoKey_Log = 1,
} eTicosLogErrorInfoKey;

//! For events with type kTicosEventType_Logs, the EventInfo contains a single array containing
//! all logs: [lvl1, msg1, lvl2, msg2, ...]

//! EventInfo dictionary keys for events with type kTicosEventType_Cdr.
typedef enum {
  kTicosCdrInfoKey_DurationMs = 1,
  kTicosCdrInfoKey_Mimetypes = 2,
  kTicosCdrInfoKey_Reason = 3,
  kTicosCdrInfoKey_Data = 4,
} eTicosCdrInfoKey;

#ifdef __cplusplus
}
#endif
