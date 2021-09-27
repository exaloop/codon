#include "lib.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include <backtrace.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

struct BacktraceFrame {
  char *function;
  char *filename;
  uintptr_t pc;
  int32_t lineno;
};

struct Backtrace {
  static const size_t LIMIT = 20;
  struct BacktraceFrame *frames;
  size_t count;

  void push_back(const char *function, const char *filename, uintptr_t pc,
                 int32_t lineno) {
    if (count >= LIMIT || !function || !filename) {
      return;
    } else if (count == 0) {
      frames = (BacktraceFrame *)seq_alloc(LIMIT * sizeof(*frames));
    }

    size_t functionLen = strlen(function) + 1;
    auto *functionDup = (char *)seq_alloc_atomic(functionLen);
    memcpy(functionDup, function, functionLen);

    size_t filenameLen = strlen(filename) + 1;
    auto *filenameDup = (char *)seq_alloc_atomic(filenameLen);
    memcpy(filenameDup, filename, filenameLen);

    frames[count++] = {functionDup, filenameDup, pc, lineno};
  }

  void free() {
    for (auto i = 0; i < count; i++) {
      auto *frame = &frames[i];
      seq_free(frame->function);
      seq_free(frame->filename);
    }
    seq_free(frames);
    frames = nullptr;
    count = 0;
  }
};

void seq_backtrace_error_callback(void *data, const char *msg, int errnum) {
  // nothing to do
}

int seq_backtrace_full_callback(void *data, uintptr_t pc, const char *filename,
                                int lineno, const char *function) {
  auto *bt = ((Backtrace *)data);
  bt->push_back(function, filename, pc, lineno);
  return (bt->count < Backtrace::LIMIT) ? 0 : 1;
}

/*
 * This is largely based on
 * llvm/examples/ExceptionDemo/ExceptionDemo.cpp
 */

namespace {
template <typename Type_> static uintptr_t ReadType(const uint8_t *&p) {
  Type_ value;
  memcpy(&value, p, sizeof(Type_));
  p += sizeof(Type_);
  return static_cast<uintptr_t>(value);
}
} // namespace

static int64_t ourBaseFromUnwindOffset;

static const unsigned char ourBaseExcpClassChars[] = {'o', 'b', 'j', '\0',
                                                      's', 'e', 'q', '\0'};

static uint64_t genClass(const unsigned char classChars[], size_t classCharsSize) {
  uint64_t ret = classChars[0];

  for (unsigned i = 1; i < classCharsSize; i++) {
    ret <<= 8;
    ret += classChars[i];
  }

  return ret;
}

static uint64_t ourBaseExceptionClass = 0;

struct OurExceptionType_t {
  int type;
};

struct OurBaseException_t {
  OurExceptionType_t type; // Seq exception type
  void *obj;               // Seq exception instance
  Backtrace bt;
  _Unwind_Exception unwindException;
};

typedef struct OurBaseException_t OurException;

struct SeqExcHeader_t {
  seq_str_t type;
  seq_str_t msg;
  seq_str_t func;
  seq_str_t file;
  seq_int_t line;
  seq_int_t col;
};

void seq_exc_init() {
  ourBaseFromUnwindOffset = seq_exc_offset();
  ourBaseExceptionClass = seq_exc_class();
}

static void seq_delete_exc(_Unwind_Exception *expToDelete) {
  if (!expToDelete || expToDelete->exception_class != ourBaseExceptionClass)
    return;
  auto *exc = (OurException *)((char *)expToDelete + ourBaseFromUnwindOffset);
  if (debug) {
    exc->bt.free();
  }
  seq_free(exc);
}

static void seq_delete_unwind_exc(_Unwind_Reason_Code reason,
                                  _Unwind_Exception *expToDelete) {
  seq_delete_exc(expToDelete);
}

static struct backtrace_state *state = nullptr;

SEQ_FUNC void *seq_alloc_exc(int type, void *obj) {
  const size_t size = sizeof(OurException);
  auto *e = (OurException *)memset(seq_alloc(size), 0, size);
  assert(e);
  e->type.type = type;
  e->obj = obj;
  e->unwindException.exception_class = ourBaseExceptionClass;
  e->unwindException.exception_cleanup = seq_delete_unwind_exc;
  if (debug) {
    e->bt.frames = nullptr;
    e->bt.count = 0;
    if (!state)
      state = backtrace_create_state(/*filename=*/nullptr, /*threaded=*/0,
                                     seq_backtrace_error_callback, /*data=*/nullptr);
    backtrace_full(state, /*skip=*/1, seq_backtrace_full_callback,
                   seq_backtrace_error_callback, &e->bt);
  }
  return &(e->unwindException);
}

static void print_from_last_dot(seq_str_t s) {
  char *p = s.str;
  int64_t n = s.len;

  for (int64_t i = n - 1; i >= 0; i--) {
    if (p[i] == '.') {
      p += (i + 1);
      n -= (i + 1);
      break;
    }
  }

  fwrite(p, 1, (size_t)n, stderr);
}

SEQ_FUNC void seq_terminate(void *exc) {
  auto *base = (OurBaseException_t *)((char *)exc + seq_exc_offset());
  void *obj = base->obj;
  auto *hdr = (SeqExcHeader_t *)obj;

  if (std::string(hdr->type.str, hdr->type.len) ==
      "std.internal.types.error.SystemExit") {
    seq_int_t status = *(seq_int_t *)(hdr + 1);
    exit((int)status);
  }

  fprintf(stderr, "\033[1m");
  print_from_last_dot(hdr->type);
  if (hdr->msg.len > 0) {
    fprintf(stderr, ": ");
    fprintf(stderr, "\033[0m");
    fwrite(hdr->msg.str, 1, (size_t)hdr->msg.len, stderr);
  } else {
    fprintf(stderr, "\033[0m");
  }

  fprintf(stderr, "\n\n");
  fprintf(stderr, "\033[1mRaised from:\033[0m \033[32m");
  fwrite(hdr->func.str, 1, (size_t)hdr->func.len, stderr);
  fprintf(stderr, "\033[0m\n");
  fwrite(hdr->file.str, 1, (size_t)hdr->file.len, stderr);
  if (hdr->line > 0) {
    fprintf(stderr, ":%lld", (long long)hdr->line);
    if (hdr->col > 0)
      fprintf(stderr, ":%lld", (long long)hdr->col);
  }
  fprintf(stderr, "\n");

  if (debug) {
    auto *bt = &base->bt;
    if (bt->count > 0) {
      fprintf(stderr, "\n\033[1mBacktrace:\033[0m\n");
      for (unsigned i = 0; i < bt->count; i++) {
        auto *frame = &bt->frames[i];
        fprintf(stderr, "  [\033[33m0x%lx\033[0m] \033[32m%s\033[0m %s:%d\n", frame->pc,
                frame->function, frame->filename, frame->lineno);
      }
    }
  }

  abort();
}

SEQ_FUNC void seq_throw(void *exc) {
  _Unwind_Reason_Code code = _Unwind_RaiseException((_Unwind_Exception *)exc);
  (void)code;
  seq_terminate(exc);
}

static uintptr_t readULEB128(const uint8_t **data) {
  uintptr_t result = 0;
  uintptr_t shift = 0;
  unsigned char byte;
  const uint8_t *p = *data;

  do {
    byte = *p++;
    result |= (byte & 0x7f) << shift;
    shift += 7;
  } while (byte & 0x80);

  *data = p;

  return result;
}

static uintptr_t readSLEB128(const uint8_t **data) {
  uintptr_t result = 0;
  uintptr_t shift = 0;
  unsigned char byte;
  const uint8_t *p = *data;

  do {
    byte = *p++;
    result |= (byte & 0x7f) << shift;
    shift += 7;
  } while (byte & 0x80);

  *data = p;

  if ((byte & 0x40) && (shift < (sizeof(result) << 3))) {
    result |= (~0 << shift);
  }

  return result;
}

static unsigned getEncodingSize(uint8_t encoding) {
  if (encoding == llvm::dwarf::DW_EH_PE_omit)
    return 0;

  switch (encoding & 0x0F) {
  case llvm::dwarf::DW_EH_PE_absptr:
    return sizeof(uintptr_t);
  case llvm::dwarf::DW_EH_PE_udata2:
    return sizeof(uint16_t);
  case llvm::dwarf::DW_EH_PE_udata4:
    return sizeof(uint32_t);
  case llvm::dwarf::DW_EH_PE_udata8:
    return sizeof(uint64_t);
  case llvm::dwarf::DW_EH_PE_sdata2:
    return sizeof(int16_t);
  case llvm::dwarf::DW_EH_PE_sdata4:
    return sizeof(int32_t);
  case llvm::dwarf::DW_EH_PE_sdata8:
    return sizeof(int64_t);
  default:
    // not supported
    abort();
  }
}

static uintptr_t readEncodedPointer(const uint8_t **data, uint8_t encoding) {
  uintptr_t result = 0;
  const uint8_t *p = *data;

  if (encoding == llvm::dwarf::DW_EH_PE_omit)
    return result;

  // first get value
  switch (encoding & 0x0F) {
  case llvm::dwarf::DW_EH_PE_absptr:
    result = ReadType<uintptr_t>(p);
    break;
  case llvm::dwarf::DW_EH_PE_uleb128:
    result = readULEB128(&p);
    break;
    // Note: This case has not been tested
  case llvm::dwarf::DW_EH_PE_sleb128:
    result = readSLEB128(&p);
    break;
  case llvm::dwarf::DW_EH_PE_udata2:
    result = ReadType<uint16_t>(p);
    break;
  case llvm::dwarf::DW_EH_PE_udata4:
    result = ReadType<uint32_t>(p);
    break;
  case llvm::dwarf::DW_EH_PE_udata8:
    result = ReadType<uint64_t>(p);
    break;
  case llvm::dwarf::DW_EH_PE_sdata2:
    result = ReadType<int16_t>(p);
    break;
  case llvm::dwarf::DW_EH_PE_sdata4:
    result = ReadType<int32_t>(p);
    break;
  case llvm::dwarf::DW_EH_PE_sdata8:
    result = ReadType<int64_t>(p);
    break;
  default:
    // not supported
    abort();
  }

  // then add relative offset
  switch (encoding & 0x70) {
  case llvm::dwarf::DW_EH_PE_absptr:
    // do nothing
    break;
  case llvm::dwarf::DW_EH_PE_pcrel:
    result += (uintptr_t)(*data);
    break;
  case llvm::dwarf::DW_EH_PE_textrel:
  case llvm::dwarf::DW_EH_PE_datarel:
  case llvm::dwarf::DW_EH_PE_funcrel:
  case llvm::dwarf::DW_EH_PE_aligned:
  default:
    // not supported
    abort();
  }

  // then apply indirection
  if (encoding & llvm::dwarf::DW_EH_PE_indirect) {
    result = *((uintptr_t *)result);
  }

  *data = p;

  return result;
}

static bool handleActionValue(int64_t *resultAction, uint8_t TTypeEncoding,
                              const uint8_t *ClassInfo, uintptr_t actionEntry,
                              uint64_t exceptionClass,
                              _Unwind_Exception *exceptionObject) {
  bool ret = false;

  if (!resultAction || !exceptionObject || (exceptionClass != ourBaseExceptionClass))
    return ret;

  auto *excp = (struct OurBaseException_t *)(((char *)exceptionObject) +
                                             ourBaseFromUnwindOffset);
  OurExceptionType_t *excpType = &(excp->type);
  seq_int_t type = excpType->type;

  const uint8_t *actionPos = (uint8_t *)actionEntry, *tempActionPos;
  int64_t typeOffset = 0, actionOffset;

  for (int i = 0;; i++) {
    // Each emitted dwarf action corresponds to a 2 tuple of
    // type info address offset, and action offset to the next
    // emitted action.
    typeOffset = (int64_t)readSLEB128(&actionPos);
    tempActionPos = actionPos;
    actionOffset = (int64_t)readSLEB128(&tempActionPos);

    assert(typeOffset >= 0);

    // Note: A typeOffset == 0 implies that a cleanup llvm.eh.selector
    //       argument has been matched.
    if (typeOffset > 0) {
      unsigned EncSize = getEncodingSize(TTypeEncoding);
      const uint8_t *EntryP = ClassInfo - typeOffset * EncSize;
      uintptr_t P = readEncodedPointer(&EntryP, TTypeEncoding);
      auto *ThisClassInfo = reinterpret_cast<OurExceptionType_t *>(P);
      // type=0 means catch-all
      if (ThisClassInfo->type == 0 || ThisClassInfo->type == type) {
        *resultAction = i + 1;
        ret = true;
        break;
      }
    }

    if (!actionOffset)
      break;

    actionPos += actionOffset;
  }

  return ret;
}

static _Unwind_Reason_Code handleLsda(int version, const uint8_t *lsda,
                                      _Unwind_Action actions, uint64_t exceptionClass,
                                      _Unwind_Exception *exceptionObject,
                                      _Unwind_Context *context) {
  _Unwind_Reason_Code ret = _URC_CONTINUE_UNWIND;

  if (!lsda)
    return ret;

  // Get the current instruction pointer and offset it before next
  // instruction in the current frame which threw the exception.
  uintptr_t pc = _Unwind_GetIP(context) - 1;

  // Get beginning current frame's code (as defined by the
  // emitted dwarf code)
  uintptr_t funcStart = _Unwind_GetRegionStart(context);
  uintptr_t pcOffset = pc - funcStart;
  const uint8_t *ClassInfo = nullptr;

  // Note: See JITDwarfEmitter::EmitExceptionTable(...) for corresponding
  //       dwarf emission

  // Parse LSDA header.
  uint8_t lpStartEncoding = *lsda++;

  if (lpStartEncoding != llvm::dwarf::DW_EH_PE_omit) {
    readEncodedPointer(&lsda, lpStartEncoding);
  }

  uint8_t ttypeEncoding = *lsda++;
  uintptr_t classInfoOffset;

  if (ttypeEncoding != llvm::dwarf::DW_EH_PE_omit) {
    // Calculate type info locations in emitted dwarf code which
    // were flagged by type info arguments to llvm.eh.selector
    // intrinsic
    classInfoOffset = readULEB128(&lsda);
    ClassInfo = lsda + classInfoOffset;
  }

  // Walk call-site table looking for range that
  // includes current PC.

  uint8_t callSiteEncoding = *lsda++;
  auto callSiteTableLength = (uint32_t)readULEB128(&lsda);
  const uint8_t *callSiteTableStart = lsda;
  const uint8_t *callSiteTableEnd = callSiteTableStart + callSiteTableLength;
  const uint8_t *actionTableStart = callSiteTableEnd;
  const uint8_t *callSitePtr = callSiteTableStart;

  while (callSitePtr < callSiteTableEnd) {
    uintptr_t start = readEncodedPointer(&callSitePtr, callSiteEncoding);
    uintptr_t length = readEncodedPointer(&callSitePtr, callSiteEncoding);
    uintptr_t landingPad = readEncodedPointer(&callSitePtr, callSiteEncoding);

    // Note: Action value
    uintptr_t actionEntry = readULEB128(&callSitePtr);

    if (exceptionClass != ourBaseExceptionClass) {
      // We have been notified of a foreign exception being thrown,
      // and we therefore need to execute cleanup landing pads
      actionEntry = 0;
    }

    if (landingPad == 0) {
      continue; // no landing pad for this entry
    }

    if (actionEntry) {
      actionEntry += (uintptr_t)actionTableStart - 1;
    }

    bool exceptionMatched = false;

    if ((start <= pcOffset) && (pcOffset < (start + length))) {
      int64_t actionValue = 0;

      if (actionEntry) {
        exceptionMatched =
            handleActionValue(&actionValue, ttypeEncoding, ClassInfo, actionEntry,
                              exceptionClass, exceptionObject);
      }

      if (!(actions & _UA_SEARCH_PHASE)) {
        // Found landing pad for the PC.
        // Set Instruction Pointer to so we re-enter function
        // at landing pad. The landing pad is created by the
        // compiler to take two parameters in registers.
        _Unwind_SetGR(context, __builtin_eh_return_data_regno(0),
                      (uintptr_t)exceptionObject);

        // Note: this virtual register directly corresponds
        //       to the return of the llvm.eh.selector intrinsic
        if (!actionEntry || !exceptionMatched) {
          // We indicate cleanup only
          _Unwind_SetGR(context, __builtin_eh_return_data_regno(1), 0);
        } else {
          // Matched type info index of llvm.eh.selector intrinsic
          // passed here.
          _Unwind_SetGR(context, __builtin_eh_return_data_regno(1),
                        (uintptr_t)actionValue);
        }

        // To execute landing pad set here
        _Unwind_SetIP(context, funcStart + landingPad);
        ret = _URC_INSTALL_CONTEXT;
      } else if (exceptionMatched) {
        ret = _URC_HANDLER_FOUND;
      }

      break;
    }
  }

  return ret;
}

SEQ_FUNC _Unwind_Reason_Code seq_personality(int version, _Unwind_Action actions,
                                             uint64_t exceptionClass,
                                             _Unwind_Exception *exceptionObject,
                                             _Unwind_Context *context) {
  const auto *lsda = (uint8_t *)_Unwind_GetLanguageSpecificData(context);
  // The real work of the personality function is captured here
  return handleLsda(version, lsda, actions, exceptionClass, exceptionObject, context);
}

SEQ_FUNC int64_t seq_exc_offset() {
  static OurBaseException_t dummy = {};
  return (int64_t)((uintptr_t)&dummy - (uintptr_t) & (dummy.unwindException));
}

SEQ_FUNC uint64_t seq_exc_class() {
  return genClass(ourBaseExcpClassChars, sizeof(ourBaseExcpClassChars));
}
