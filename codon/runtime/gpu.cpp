// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "lib.h"

#ifdef CODON_GPU

#include "cuda.h"

#define fail(err)                                                                      \
  do {                                                                                 \
    const char *msg;                                                                   \
    cuGetErrorString((err), &msg);                                                     \
    fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, msg);             \
    abort();                                                                           \
  } while (0)

#define check(call)                                                                    \
  do {                                                                                 \
    auto err = (call);                                                                 \
    if (err != CUDA_SUCCESS) {                                                         \
      fail(err);                                                                       \
    }                                                                                  \
  } while (0)

static std::vector<CUmodule> modules;
static CUcontext context;

void seq_nvptx_init() {
  CUdevice device;
  check(cuInit(0));
  check(cuDeviceGet(&device, 0));
  check(cuCtxCreate(&context, 0, device));
}

SEQ_FUNC void seq_nvptx_load_module(const char *filename) {
  CUmodule module;
  check(cuModuleLoad(&module, filename));
  modules.push_back(module);
}

SEQ_FUNC seq_int_t seq_nvptx_device_count() {
  int devCount;
  check(cuDeviceGetCount(&devCount));
  return devCount;
}

SEQ_FUNC seq_str_t seq_nvptx_device_name(CUdevice device) {
  char name[128];
  check(cuDeviceGetName(name, sizeof(name) - 1, device));
  auto sz = static_cast<seq_int_t>(strlen(name));
  auto *p = (char *)seq_alloc_atomic(sz);
  memcpy(p, name, sz);
  return {sz, p};
}

SEQ_FUNC seq_int_t seq_nvptx_device_capability(CUdevice device) {
  int devMajor, devMinor;
  check(cuDeviceComputeCapability(&devMajor, &devMinor, device));
  return ((seq_int_t)devMajor << 32) | (seq_int_t)devMinor;
}

SEQ_FUNC CUdevice seq_nvptx_device(seq_int_t idx) {
  CUdevice device;
  check(cuDeviceGet(&device, idx));
  return device;
}

static bool name_char_valid(char c, bool first) {
  bool ok = ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
  if (!first)
    ok = ok || ('0' <= c && c <= '9');
  return ok;
}

SEQ_FUNC CUfunction seq_nvptx_function(seq_str_t name) {
  CUfunction function;
  CUresult result;

  std::vector<char> clean(name.len + 1);
  for (unsigned i = 0; i < name.len; i++) {
    char c = name.str[i];
    clean[i] = (name_char_valid(c, i == 0) ? c : '_');
  }
  clean[name.len] = '\0';

  for (auto it = modules.rbegin(); it != modules.rend(); ++it) {
    result = cuModuleGetFunction(&function, *it, clean.data());
    if (result == CUDA_SUCCESS) {
      return function;
    } else if (result == CUDA_ERROR_NOT_FOUND) {
      continue;
    } else {
      break;
    }
  }

  fail(result);
  return {};
}

SEQ_FUNC void seq_nvptx_invoke(CUfunction f, unsigned int gridDimX,
                               unsigned int gridDimY, unsigned int gridDimZ,
                               unsigned int blockDimX, unsigned int blockDimY,
                               unsigned int blockDimZ, unsigned int sharedMemBytes,
                               void **kernelParams) {
  check(cuLaunchKernel(f, gridDimX, gridDimY, gridDimZ, blockDimX, blockDimY, blockDimZ,
                       sharedMemBytes, nullptr, kernelParams, nullptr));
}

SEQ_FUNC CUdeviceptr seq_nvptx_device_alloc(seq_int_t size) {
  if (size == 0)
    return {};

  CUdeviceptr devp;
  check(cuMemAlloc(&devp, size));
  return devp;
}

SEQ_FUNC void seq_nvptx_memcpy_h2d(CUdeviceptr devp, char *hostp, seq_int_t size) {
  if (size)
    check(cuMemcpyHtoD(devp, hostp, size));
}

SEQ_FUNC void seq_nvptx_memcpy_d2h(char *hostp, CUdeviceptr devp, seq_int_t size) {
  if (size)
    check(cuMemcpyDtoH(hostp, devp, size));
}

SEQ_FUNC void seq_nvptx_device_free(CUdeviceptr devp) {
  if (devp)
    check(cuMemFree(devp));
}

#endif /* CODON_GPU */
