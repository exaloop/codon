import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/k2mm/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
alpha = 1.5
beta = 1.2
original_A = np.load(directory + '/input-A.npy')
A = np.empty_like(original_A)
np.copyto(A, original_A)
original_B = np.load(directory + '/input-B.npy')
B = np.empty_like(original_B)
np.copyto(B, original_B)
original_C = np.load(directory + '/input-C.npy')
C = np.empty_like(original_C)
np.copyto(C, original_C)
original_D = np.load(directory + '/input-D.npy')
D = np.empty_like(original_D)
np.copyto(D, original_D)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(alpha, beta, A, B, C, D):
    started = time.perf_counter()
    result = kernel_module.kernel(alpha, beta, A, B, C, D)
    elapsed = time.perf_counter() - started
    emit(D, directory + '/output-D')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_A = A.ctypes.data
    address_B = B.ctypes.data
    address_C = C.ctypes.data
    address_D = D.ctypes.data
    for iteration in range(32):
        np.copyto(A, original_A)
        assert A.ctypes.data == address_A
        np.copyto(B, original_B)
        assert B.ctypes.data == address_B
        np.copyto(C, original_C)
        assert C.ctypes.data == address_C
        np.copyto(D, original_D)
        assert D.ctypes.data == address_D
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(A, original_A)
        np.copyto(B, original_B)
        np.copyto(C, original_C)
        np.copyto(D, original_D)
        elapsed = sample(alpha, beta, A, B, C, D)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
