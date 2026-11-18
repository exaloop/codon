import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/syrk/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
alpha = 1.5
beta = 1.2
original_C = np.load(directory + '/input-C.npy')
C = np.empty_like(original_C)
np.copyto(C, original_C)
original_A = np.load(directory + '/input-A.npy')
A = np.empty_like(original_A)
np.copyto(A, original_A)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(alpha, beta, C, A):
    started = time.perf_counter()
    result = kernel_module.kernel(alpha, beta, C, A)
    elapsed = time.perf_counter() - started
    emit(C, directory + '/output-C')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_C = C.ctypes.data
    address_A = A.ctypes.data
    for iteration in range(32):
        np.copyto(C, original_C)
        assert C.ctypes.data == address_C
        np.copyto(A, original_A)
        assert A.ctypes.data == address_A
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(C, original_C)
        np.copyto(A, original_A)
        elapsed = sample(alpha, beta, C, A)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
