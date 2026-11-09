import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/mlp/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
original_input = np.load(directory + '/input-input.npy')
input = np.empty_like(original_input)
np.copyto(input, original_input)
original_w1 = np.load(directory + '/input-w1.npy')
w1 = np.empty_like(original_w1)
np.copyto(w1, original_w1)
original_b1 = np.load(directory + '/input-b1.npy')
b1 = np.empty_like(original_b1)
np.copyto(b1, original_b1)
original_w2 = np.load(directory + '/input-w2.npy')
w2 = np.empty_like(original_w2)
np.copyto(w2, original_w2)
original_b2 = np.load(directory + '/input-b2.npy')
b2 = np.empty_like(original_b2)
np.copyto(b2, original_b2)
original_w3 = np.load(directory + '/input-w3.npy')
w3 = np.empty_like(original_w3)
np.copyto(w3, original_w3)
original_b3 = np.load(directory + '/input-b3.npy')
b3 = np.empty_like(original_b3)
np.copyto(b3, original_b3)
with open('/proc/self/smaps_rollup') as memory_status:
    for record in memory_status:
        if record.startswith('AnonHugePages:'):
            print('BENCH_HUGEPAGES_KIB', int(record.split()[1]))

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(input, w1, b1, w2, b2, w3, b3):
    started = time.perf_counter()
    result = kernel_module.mlp(input, w1, b1, w2, b2, w3, b3)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_input = input.ctypes.data
    address_w1 = w1.ctypes.data
    address_b1 = b1.ctypes.data
    address_w2 = w2.ctypes.data
    address_b2 = b2.ctypes.data
    address_w3 = w3.ctypes.data
    address_b3 = b3.ctypes.data
    for iteration in range(32):
        np.copyto(input, original_input)
        assert input.ctypes.data == address_input
        np.copyto(w1, original_w1)
        assert w1.ctypes.data == address_w1
        np.copyto(b1, original_b1)
        assert b1.ctypes.data == address_b1
        np.copyto(w2, original_w2)
        assert w2.ctypes.data == address_w2
        np.copyto(b2, original_b2)
        assert b2.ctypes.data == address_b2
        np.copyto(w3, original_w3)
        assert w3.ctypes.data == address_w3
        np.copyto(b3, original_b3)
        assert b3.ctypes.data == address_b3
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(input, w1, b1, w2, b2, w3, b3)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(input, original_input)
        np.copyto(w1, original_w1)
        np.copyto(b1, original_b1)
        np.copyto(w2, original_w2)
        np.copyto(b2, original_b2)
        np.copyto(w3, original_w3)
        np.copyto(b3, original_b3)
        elapsed = sample(input, w1, b1, w2, b2, w3, b3)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
