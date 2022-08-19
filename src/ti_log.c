// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "ti_span_private.h"
#include <ti_config.h>
#include <ti_http.h>
#include <ti_http_transport.h>
#include <ti_log.h>
#include <ti_span.h>
#include <ti_http_internal.h>
#include <ti_log_internal.h>

#include <stddef.h>

#include <_ti_cfg.h>

#ifndef TI_NO_LOGGING

// Only using volatile here, not for thread safety, but so that the compiler does not optimize what
// it falsely thinks are stale reads.
static ti_log_message_fn volatile _ti_log_message_callback = NULL;
static ti_log_classification_filter_fn volatile _ti_message_filter_callback = NULL;

void ti_log_set_message_callback(ti_log_message_fn log_message_callback)
{
  // We assume assignments are atomic for the supported platforms and compilers.
  _ti_log_message_callback = log_message_callback;
}

void ti_log_set_classification_filter_callback(
    ti_log_classification_filter_fn message_filter_callback)
{
  // We assume assignments are atomic for the supported platforms and compilers.
  _ti_message_filter_callback = message_filter_callback;
}

TI_INLINE ti_log_message_fn _ti_log_get_message_callback(ti_log_classification classification)
{
  _ti_PRECONDITION(classification > 0);

  // Copy the volatile fields to local variables so that they don't change within this function.
  ti_log_message_fn const message_callback = _ti_log_message_callback;
  ti_log_classification_filter_fn const message_filter_callback = _ti_message_filter_callback;

  // If the user hasn't registered a message_filter_callback, then we log everything, as long as a
  // message_callback method was provided.
  // Otherwise, we log only what that filter allows.
  if (message_callback != NULL
      && (message_filter_callback == NULL || message_filter_callback(classification)))
  {
    return message_callback;
  }

  // This message's classification is either not allowed by the filter, or there is no callback
  // function registered to receive the message. In both cases, we should not log it.
  return NULL;
}

// This function returns whether or not the passed-in message should be logged.
bool _ti_log_should_write(ti_log_classification classification)
{
  return _ti_log_get_message_callback(classification) != NULL;
}

// This function attempts to log the passed-in message.
void _ti_log_write(ti_log_classification classification, ti_span message)
{
  _ti_PRECONDITION_VALID_SPAN(message, 0, true);

  ti_log_message_fn const message_callback = _ti_log_get_message_callback(classification);

  if (message_callback != NULL)
  {
    message_callback(classification, message);
  }
}

#endif // TI_NO_LOGGING
