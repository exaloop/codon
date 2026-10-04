// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/runtime/lib.h"

namespace codon {
namespace runtime {

enum TypeInfoField : unsigned {
  TYPE_ID,
  MRO_COUNT,
  MRO_IDS,
  MRO_OFFSETS,
  SORTED_MRO_IDS,
  SLOT_COUNT,
  SLOTS,
  METHOD_COUNT,
  METHODS,
  POLYMORPHIC,
  RAW_NAME,
  NICE_NAME,
  BASE_NAME,
  PARAMETER_COUNT,
  PARAMETERS,
  VALUE_SIZE,
  REFERENCE,
  EXCEPTION_OFFSET,
  SYSTEM_EXIT_OFFSET
};

struct TypeSlot {
  seq_str_t name;
  seq_int_t type;
  seq_int_t offset;
};

struct TypeInfo {
  seq_int_t id;
  seq_int_t n_mro;
  const seq_int_t *mro;
  const seq_int_t *offsets;
  const seq_int_t *sorted_mro;
  seq_int_t n_slots;
  const TypeSlot *slots;
  seq_int_t n_methods;
  void *const *methods;
  bool rtti;
  seq_str_t raw_name;
  seq_str_t nice_name;
  seq_str_t base_name;
  seq_int_t n_params;
  const seq_int_t *params;
  seq_int_t size;
  bool reference;
  seq_int_t exception_offset;
  seq_int_t system_exit_offset;
};

} // namespace runtime
} // namespace codon
