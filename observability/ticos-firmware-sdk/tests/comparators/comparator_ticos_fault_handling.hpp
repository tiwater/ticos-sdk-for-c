//! @file
//!
//! Comparators for fault_handling.h types.

#pragma once

#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "ticos/panics/fault_handling.h"
}

class Tcs_sTicosAssertInfo_Comparator : public MockNamedValueComparator {
 public:
  virtual bool isEqual(const void *object1, const void *object2) {
    const sTicosAssertInfo *a = (const sTicosAssertInfo *)object1;
    const sTicosAssertInfo *b = (const sTicosAssertInfo *)object2;

    return (a->extra == b->extra) && (a->assert_reason == b->assert_reason);
  }
  virtual SimpleString valueToString(const void *object) {
    const sTicosAssertInfo *o = (const sTicosAssertInfo *)object;
    return StringFromFormat("sTicosAssertInfo:") +
           StringFromFormat(" extra: 0x%08" PRIx32, o->extra) +
           StringFromFormat(" assert_reason: %d", o->assert_reason);
  }
};
