// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Context for canceling long running operations.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_CONTEXT_H
#define _ti_CONTEXT_H

#include <ti_result.h>

#include <stddef.h>
#include <stdint.h>

#include <_ti_cfg_prefix.h>

/**
 * @brief A context is a node within a tree that represents expiration times and key/value pairs.
 */
// Definition is below. Defining the typedef first is necessary here since there is a cycle.
typedef struct ti_context ti_context;

/**
 * @brief A context is a node within a tree that represents expiration times and key/value pairs.
 *
 * @details The root node in the tree (ultimate parent).
 */
struct ti_context
{
  struct
  {
    ti_context const* parent; // Pointer to parent context (or NULL); immutable after creation
    int64_t expiration; // Time when context expires
    void const* key; // Pointers to the key & value (usually NULL)
    void const* value;
  } _internal;
};

#define _ti_CONTEXT_MAX_EXPIRATION 0x7FFFFFFFFFFFFFFF

/**
 * @brief The application root #ti_context instances.
 * @details The #ti_context_application never expires but you can explicitly cancel it by passing
 * its address to #ti_context_cancel() which effectively cancels all its #ti_context child nodes.
 */
extern ti_context ti_context_application;

/**
 * @brief Creates a new expiring #ti_context node that is a child of the specified parent.
 *
 * @param[in] parent The #ti_context node that is the parent to the new node.
 * @param[in] expiration The time when this new node should be canceled.
 *
 * @return The new child #ti_context node.
 */
TI_NODISCARD ti_context
ti_context_create_with_expiration(ti_context const* parent, int64_t expiration);

/**
 * @brief Creates a new key/value ti_context node that is a child of the specified parent.
 *
 * @param[in] parent The #ti_context node that is the parent to the new node.
 * @param[in] key A pointer to the key of this new #ti_context node.
 * @param[in] value A pointer to the value of this new #ti_context node.
 *
 * @return The new child #ti_context node.
 */
TI_NODISCARD ti_context
ti_context_create_with_value(ti_context const* parent, void const* key, void const* value);

/**
 * @brief Cancels the specified #ti_context node; this cancels all the child nodes as well.
 *
 * @param[in,out] ref_context A pointer to the #ti_context node to be canceled.
 */
void ti_context_cancel(ti_context* ref_context);

/**
 * @brief Returns the soonest expiration time of this #ti_context node or any of its parent nodes.
 *
 * @param[in] context A pointer to an #ti_context node.
 * @return The soonest expiration time from this context and its parents.
 */
TI_NODISCARD int64_t ti_context_get_expiration(ti_context const* context);

/**
 * @brief Returns `true` if this #ti_context node or any of its parent nodes' expiration is before
 * the \p current_time.
 *
 * @param[in] context A pointer to the #ti_context node to check.
 * @param[in] current_time The current time.
 */
TI_NODISCARD bool ti_context_has_expired(ti_context const* context, int64_t current_time);

/**
 * @brief Walks up this #ti_context node's parents until it find a node whose key matches the
 * specified key and returns the corresponding value.
 *
 * @param[in] context The #ti_context node in the tree where checking starts.
 * @param[in] key A pointer to the key to be scanned for.
 * @param[out] out_value A pointer to a `void const*` that will receive the key's associated value
 * if the key is found.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK The key is found.
 * @retval #TI_ERROR_ITEM_NOT_FOUND No nodes are found with the specified key.
 */
TI_NODISCARD ti_result
ti_context_get_value(ti_context const* context, void const* key, void const** out_value);

#include <_ti_cfg_suffix.h>

#endif // _ti_CONTEXT_H
