import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/resnet/python/kernel.py')
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
original_conv2 = np.load(directory + '/input-conv2.npy')
conv2 = np.empty_like(original_conv2)
np.copyto(conv2, original_conv2)
original_conv3 = np.load(directory + '/input-conv3.npy')
conv3 = np.empty_like(original_conv3)
np.copyto(conv3, original_conv3)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(input, conv1, conv2, conv3):
    started = time.perf_counter()
    result = kernel_module.resnet_basicblock(input, conv1, conv2, conv3)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_input = input.ctypes.data
    address_conv1 = conv1.ctypes.data
    address_conv2 = conv2.ctypes.data
    address_conv3 = conv3.ctypes.data
    for iteration in range(32):
        np.copyto(input, original_input)
        assert input.ctypes.data == address_input
        np.copyto(conv1, original_conv1)
        assert conv1.ctypes.data == address_conv1
        np.copyto(conv2, original_conv2)
        assert conv2.ctypes.data == address_conv2
        np.copyto(conv3, original_conv3)
        assert conv3.ctypes.data == address_conv3
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(input, original_input)
        np.copyto(conv1, original_conv1)
        np.copyto(conv2, original_conv2)
        np.copyto(conv3, original_conv3)
        elapsed = sample(input, conv1, conv2, conv3)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
