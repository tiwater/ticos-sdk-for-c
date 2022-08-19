// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Credentials used for authentication with many (not all) Ticos SDK client libraries.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_CREDENTIALS_H
#define _ti_CREDENTIALS_H

#include <ti_http_transport.h>
#include <ti_result.h>
#include <ti_span.h>

#include <stddef.h>

#include <_ti_cfg_prefix.h>

/**
 * @brief Equivalent to no credential (`NULL`).
 */
#define TI_CREDENTIAL_ANONYMOUS NULL

/**
 * @brief Function callback definition as a contract to be implemented for a credential to set
 * authentication scopes when it is supported by the type of the credential.
 */
typedef TI_NODISCARD ti_result (
    *_ti_credential_set_scopes_fn)(void* ref_credential, ti_span scopes);

/**
 * @brief Credential definition. It is used internally to authenticate an SDK client with Ticos.
 * All types of credentials must contain this structure as their first member.
 */
typedef struct
{
  struct
  {
    _ti_http_policy_process_fn apply_credential_policy;

    /// If the credential doesn't support scopes, this function pointer is `NULL`.
    _ti_credential_set_scopes_fn set_scopes;
  } _internal;
} _ti_credential;

#include <_ti_cfg_suffix.h>

#endif // _ti_CREDENTIALS_H
