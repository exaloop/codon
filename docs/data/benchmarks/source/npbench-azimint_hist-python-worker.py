import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/azimint_hist/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
original_data = np.load(directory + '/input-data.npy')
data = np.empty_like(original_data)
np.copyto(data, original_data)
original_radius = np.load(directory + '/input-radius.npy')
radius = np.empty_like(original_radius)
np.copyto(radius, original_radius)
npt = 1000

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(data, radius, npt):
    started = time.perf_counter()
    result = kernel_module.azimint_hist(data, radius, npt)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_data = data.ctypes.data
    address_radius = radius.ctypes.data
    for iteration in range(32):
        np.copyto(data, original_data)
        assert data.ctypes.data == address_data
        np.copyto(radius, original_radius)
        assert radius.ctypes.data == address_radius
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(data, original_data)
        np.copyto(radius, original_radius)
        elapsed = sample(data, radius, npt)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
