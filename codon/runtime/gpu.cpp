#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "lib.h"

#ifdef CODON_GPU

#include "cuda.h"

#define check(call)                                                                    \
  do {                                                                                 \
    auto err = (call);                                                                 \
    if (err != CUDA_SUCCESS) {                                                         \
      const char *msg;                                                                 \
      cuGetErrorString(err, &msg);                                                     \
      fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, msg);           \
      abort();                                                                         \
    }                                                                                  \
  } while (0)

static CUmodule module;

SEQ_FUNC void seq_nvptx_init(const char *filename) {
  check(cuInit(0));
  check(cuModuleLoad(&module, filename));
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

SEQ_FUNC CUfunction seq_nvptx_function(const char *name) {
  CUfunction function;
  check(cuModuleGetFunction(&function, module, name));
  return function;
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
  CUdeviceptr devp;
  check(cuMemAlloc(&devp, size));
  return devp;
}

SEQ_FUNC void seq_nvptx_memcpy_h2d(CUdeviceptr devp, char *hostp, seq_int_t size) {
  check(cuMemcpyHtoD(devp, hostp, size));
}

SEQ_FUNC void seq_nvptx_memcpy_d2h(char *hostp, CUdeviceptr devp, seq_int_t size) {
  check(cuMemcpyDtoH(hostp, devp, size));
}

SEQ_FUNC void seq_nvptx_device_free(CUdeviceptr devp) { check(cuMemFree(devp)); }

#endif /* CODON_GPU */
