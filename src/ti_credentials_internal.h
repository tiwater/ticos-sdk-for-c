// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines internals used by credentials.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_CREDENTIALS_INTERNAL_H
#define _ti_CREDENTIALS_INTERNAL_H

#include <ti_credentials.h>
#include <ti_result.h>
#include <ti_span.h>

#include <stddef.h>

#include <_ti_cfg_prefix.h>

TI_INLINE TI_NODISCARD ti_result
_ti_credential_set_scopes(_ti_credential* credential, ti_span scopes)
{
  return (credential == NULL || credential->_internal.set_scopes == NULL)
      ? TI_OK
      : (credential->_internal.set_scopes)(credential, scopes);
}

#include <_ti_cfg_suffix.h>

#endif // _ti_CREDENTIALS_INTERNAL_H
