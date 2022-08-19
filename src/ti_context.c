// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_context.h>
#include <ti_precondition_internal.h>

#include <stddef.h>

#include <_ti_cfg.h>

// This is a global ti_context node representing the entire application. By default, this node
// never expires. Call ti_context_cancel passing a pointer to this node to cancel the entire
// application (which cancels all the child nodes).
ti_context ti_context_application = {
  ._internal
  = { .parent = NULL, .expiration = _ti_CONTEXT_MAX_EXPIRATION, .key = NULL, .value = NULL }
};

// Returns the soonest expiration time of this ti_context node or any of its parent nodes.
TI_NODISCARD int64_t ti_context_get_expiration(ti_context const* context)
{
  _ti_PRECONDITION_NOT_NULL(context);

  int64_t expiration = _ti_CONTEXT_MAX_EXPIRATION;
  for (; context != NULL; context = context->_internal.parent)
  {
    if (context->_internal.expiration < expiration)
    {
      expiration = context->_internal.expiration;
    }
  }
  return expiration;
}

// Walks up this ti_context node's parent until it find a node whose key matches the specified key
// and return the corresponding value. Returns TI_ERROR_ITEM_NOT_FOUND is there are no nodes
// matching the specified key.
TI_NODISCARD ti_result
ti_context_get_value(ti_context const* context, void const* key, void const** out_value)
{
  _ti_PRECONDITION_NOT_NULL(context);
  _ti_PRECONDITION_NOT_NULL(out_value);
  _ti_PRECONDITION_NOT_NULL(key);

  for (; context != NULL; context = context->_internal.parent)
  {
    if (context->_internal.key == key)
    {
      *out_value = context->_internal.value;
      return TI_OK;
    }
  }
  *out_value = NULL;
  return TI_ERROR_ITEM_NOT_FOUND;
}

TI_NODISCARD ti_context
ti_context_create_with_expiration(ti_context const* parent, int64_t expiration)
{
  _ti_PRECONDITION_NOT_NULL(parent);
  _ti_PRECONDITION(expiration >= 0);

  return (ti_context){ ._internal = { .parent = parent, .expiration = expiration } };
}

TI_NODISCARD ti_context
ti_context_create_with_value(ti_context const* parent, void const* key, void const* value)
{
  _ti_PRECONDITION_NOT_NULL(parent);
  _ti_PRECONDITION_NOT_NULL(key);

  return (ti_context){
    ._internal
    = { .parent = parent, .expiration = _ti_CONTEXT_MAX_EXPIRATION, .key = key, .value = value }
  };
}

void ti_context_cancel(ti_context* ref_context)
{
  _ti_PRECONDITION_NOT_NULL(ref_context);

  ref_context->_internal.expiration = 0; // The beginning of time
}

TI_NODISCARD bool ti_context_has_expired(ti_context const* context, int64_t current_time)
{
  _ti_PRECONDITION_NOT_NULL(context);
  _ti_PRECONDITION(current_time >= 0);

  return ti_context_get_expiration(context) < current_time;
}
