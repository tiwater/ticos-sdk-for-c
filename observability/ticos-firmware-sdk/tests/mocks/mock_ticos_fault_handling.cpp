//! @file
//!
//! Mock implementation of ticos fault handling functions

#include "ticos/panics/fault_handling.h"

#include "CppUTestExt/MockSupport.h"

void ticos_fault_handling_assert_extra(void *pc, void *lr, sTicosAssertInfo *extra_info) {
  mock()
    .actualCall(__func__)
    .withPointerParameter("pc", pc)
    .withPointerParameter("lr", lr)
    .withParameterOfType("sTicosAssertInfo", "extra_info", extra_info);
}
