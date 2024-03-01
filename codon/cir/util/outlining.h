// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/cir.h"

namespace codon {
namespace ir {
namespace util {

/// The result of an outlining operation.
struct OutlineResult {
  /// Information about an argument of an outlined function.
  enum ArgKind {
    CONSTANT, ///< Argument is not modified by outlined function
    MODIFIED, ///< Argument is modified and passed by pointer
  };

  /// The outlined function
  BodiedFunc *func = nullptr;

  /// The call to the outlined function
  CallInstr *call = nullptr;

  /// Information about each argument of the outlined function.
  /// "CONSTANT" arguments are passed by value; "MODIFIED"
  /// arguments are passed by pointer and written to by the
  /// outlined function. The size of this vector is the same
  /// as the number of arguments of the outlined function; each
  /// entry corresponds to one of those arguments.
  std::vector<ArgKind> argKinds;

  /// Number of externally-handled control flows.
  /// For example, an outlined function that contains a "break"
  /// of a non-outlined loop will return an integer code that
  /// tells the callee to perform this break. A series of
  /// if-statements are added to the call site to check the
  /// returned code and perform the correct action. This value
  /// is the number of if-statements generated. If it is zero,
  /// the function returns void and no such checks are done.
  int numOutFlows = 0;

  operator bool() const { return bool(func); }
};

/// Outlines a region of IR delineated by begin and end iterators
/// on a particular series flow. The outlined code will be replaced
/// by a call to the outlined function, and possibly extra logic if
/// control flow needs to be handled.
/// @param parent the function containing the series flow
/// @param series the series flow on which outlining will happen
/// @param begin start of outlining
/// @param end end of outlining (non-inclusive like standard iterators)
/// @param allowOutflows allow outlining regions with "out-flows"
/// @param outlineGlobals outline globals as arguments to outlined function
/// @param allByValue pass all outlined vars by value (can change semantics)
/// @return the result of outlining
OutlineResult outlineRegion(BodiedFunc *parent, SeriesFlow *series,
                            decltype(series->begin()) begin,
                            decltype(series->end()) end, bool allowOutflows = true,
                            bool outlineGlobals = false, bool allByValue = false);

/// Outlines a series flow from its parent function. The outlined code
/// will be replaced by a call to the outlined function, and possibly
/// extra logic if control flow needs to be handled.
/// @param parent the function containing the series flow
/// @param series the series flow on which outlining will happen
/// @param allowOutflows allow outlining regions with "out-flows"
/// @param outlineGlobals outline globals as arguments to outlined function
/// @param allByValue pass all outlined vars by value (can change semantics)
/// @return the result of outlining
OutlineResult outlineRegion(BodiedFunc *parent, SeriesFlow *series,
                            bool allowOutflows = true, bool outlineGlobals = false,
                            bool allByValue = false);

} // namespace util
} // namespace ir
} // namespace codon
