import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/compute/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
original_array_1 = np.load(directory + '/input-array_1.npy')
array_1 = np.empty_like(original_array_1)
np.copyto(array_1, original_array_1)
original_array_2 = np.load(directory + '/input-array_2.npy')
array_2 = np.empty_like(original_array_2)
np.copyto(array_2, original_array_2)
a = 4
b = 3
c = 9

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(array_1, array_2, a, b, c):
    started = time.perf_counter()
    result = kernel_module.compute(array_1, array_2, a, b, c)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_array_1 = array_1.ctypes.data
    address_array_2 = array_2.ctypes.data
    for iteration in range(32):
        np.copyto(array_1, original_array_1)
        assert array_1.ctypes.data == address_array_1
        np.copyto(array_2, original_array_2)
        assert array_2.ctypes.data == address_array_2
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(array_1, original_array_1)
        np.copyto(array_2, original_array_2)
        elapsed = sample(array_1, array_2, a, b, c)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
