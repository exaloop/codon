import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/lenet/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
original_input = np.load(directory + '/input-input.npy')
input = np.empty_like(original_input)
np.copyto(input, original_input)
original_conv1 = np.load(directory + '/input-conv1.npy')
conv1 = np.empty_like(original_conv1)
np.copyto(conv1, original_conv1)
original_conv1bias = np.load(directory + '/input-conv1bias.npy')
conv1bias = np.empty_like(original_conv1bias)
np.copyto(conv1bias, original_conv1bias)
original_conv2 = np.load(directory + '/input-conv2.npy')
conv2 = np.empty_like(original_conv2)
np.copyto(conv2, original_conv2)
original_conv2bias = np.load(directory + '/input-conv2bias.npy')
conv2bias = np.empty_like(original_conv2bias)
np.copyto(conv2bias, original_conv2bias)
original_fc1w = np.load(directory + '/input-fc1w.npy')
fc1w = np.empty_like(original_fc1w)
np.copyto(fc1w, original_fc1w)
original_fc1b = np.load(directory + '/input-fc1b.npy')
fc1b = np.empty_like(original_fc1b)
np.copyto(fc1b, original_fc1b)
original_fc2w = np.load(directory + '/input-fc2w.npy')
fc2w = np.empty_like(original_fc2w)
np.copyto(fc2w, original_fc2w)
original_fc2b = np.load(directory + '/input-fc2b.npy')
fc2b = np.empty_like(original_fc2b)
np.copyto(fc2b, original_fc2b)
original_fc3w = np.load(directory + '/input-fc3w.npy')
fc3w = np.empty_like(original_fc3w)
np.copyto(fc3w, original_fc3w)
original_fc3b = np.load(directory + '/input-fc3b.npy')
fc3b = np.empty_like(original_fc3b)
np.copyto(fc3b, original_fc3b)
N = 8
C_before_fc1 = 26896

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(input, conv1, conv1bias, conv2, conv2bias, fc1w, fc1b, fc2w, fc2b, fc3w, fc3b, N, C_before_fc1):
    started = time.perf_counter()
    result = kernel_module.lenet5(input, conv1, conv1bias, conv2, conv2bias, fc1w, fc1b, fc2w, fc2b, fc3w, fc3b, N, C_before_fc1)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_input = input.ctypes.data
    address_conv1 = conv1.ctypes.data
    address_conv1bias = conv1bias.ctypes.data
    address_conv2 = conv2.ctypes.data
    address_conv2bias = conv2bias.ctypes.data
    address_fc1w = fc1w.ctypes.data
    address_fc1b = fc1b.ctypes.data
    address_fc2w = fc2w.ctypes.data
    address_fc2b = fc2b.ctypes.data
    address_fc3w = fc3w.ctypes.data
    address_fc3b = fc3b.ctypes.data
    for iteration in range(32):
        np.copyto(input, original_input)
        assert input.ctypes.data == address_input
        np.copyto(conv1, original_conv1)
        assert conv1.ctypes.data == address_conv1
        np.copyto(conv1bias, original_conv1bias)
        assert conv1bias.ctypes.data == address_conv1bias
        np.copyto(conv2, original_conv2)
        assert conv2.ctypes.data == address_conv2
        np.copyto(conv2bias, original_conv2bias)
        assert conv2bias.ctypes.data == address_conv2bias
        np.copyto(fc1w, original_fc1w)
        assert fc1w.ctypes.data == address_fc1w
        np.copyto(fc1b, original_fc1b)
        assert fc1b.ctypes.data == address_fc1b
        np.copyto(fc2w, original_fc2w)
        assert fc2w.ctypes.data == address_fc2w
        np.copyto(fc2b, original_fc2b)
        assert fc2b.ctypes.data == address_fc2b
        np.copyto(fc3w, original_fc3w)
        assert fc3w.ctypes.data == address_fc3w
        np.copyto(fc3b, original_fc3b)
        assert fc3b.ctypes.data == address_fc3b
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(input, original_input)
        np.copyto(conv1, original_conv1)
        np.copyto(conv1bias, original_conv1bias)
        np.copyto(conv2, original_conv2)
        np.copyto(conv2bias, original_conv2bias)
        np.copyto(fc1w, original_fc1w)
        np.copyto(fc1b, original_fc1b)
        np.copyto(fc2w, original_fc2w)
        np.copyto(fc2b, original_fc2b)
        np.copyto(fc3w, original_fc3w)
        np.copyto(fc3b, original_fc3b)
        elapsed = sample(input, conv1, conv1bias, conv2, conv2bias, fc1w, fc1b, fc2w, fc2b, fc3w, fc3b, N, C_before_fc1)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
