import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/conv2d_bias/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
original_input = np.load(directory + '/input-input.npy')
input = np.empty_like(original_input)
np.copyto(input, original_input)
original_weights = np.load(directory + '/input-weights.npy')
weights = np.empty_like(original_weights)
np.copyto(weights, original_weights)
original_bias = np.load(directory + '/input-bias.npy')
bias = np.empty_like(original_bias)
np.copyto(bias, original_bias)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(input, weights, bias):
    started = time.perf_counter()
    result = kernel_module.conv2d_bias(input, weights, bias)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_input = input.ctypes.data
    address_weights = weights.ctypes.data
    address_bias = bias.ctypes.data
    for iteration in range(32):
        np.copyto(input, original_input)
        assert input.ctypes.data == address_input
        np.copyto(weights, original_weights)
        assert weights.ctypes.data == address_weights
        np.copyto(bias, original_bias)
        assert bias.ctypes.data == address_bias
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(input, original_input)
        np.copyto(weights, original_weights)
        np.copyto(bias, original_bias)
        elapsed = sample(input, weights, bias)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
