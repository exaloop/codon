import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/gemver/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
alpha = 1.5
beta = 1.2
original_A = np.load(directory + '/input-A.npy')
A = np.empty_like(original_A)
np.copyto(A, original_A)
original_u1 = np.load(directory + '/input-u1.npy')
u1 = np.empty_like(original_u1)
np.copyto(u1, original_u1)
original_v1 = np.load(directory + '/input-v1.npy')
v1 = np.empty_like(original_v1)
np.copyto(v1, original_v1)
original_u2 = np.load(directory + '/input-u2.npy')
u2 = np.empty_like(original_u2)
np.copyto(u2, original_u2)
original_v2 = np.load(directory + '/input-v2.npy')
v2 = np.empty_like(original_v2)
np.copyto(v2, original_v2)
original_w = np.load(directory + '/input-w.npy')
w = np.empty_like(original_w)
np.copyto(w, original_w)
original_x = np.load(directory + '/input-x.npy')
x = np.empty_like(original_x)
np.copyto(x, original_x)
original_y = np.load(directory + '/input-y.npy')
y = np.empty_like(original_y)
np.copyto(y, original_y)
original_z = np.load(directory + '/input-z.npy')
z = np.empty_like(original_z)
np.copyto(z, original_z)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(alpha, beta, A, u1, v1, u2, v2, w, x, y, z):
    started = time.perf_counter()
    result = kernel_module.kernel(alpha, beta, A, u1, v1, u2, v2, w, x, y, z)
    elapsed = time.perf_counter() - started
    emit(A, directory + '/output-A')
    emit(w, directory + '/output-w')
    emit(x, directory + '/output-x')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_A = A.ctypes.data
    address_u1 = u1.ctypes.data
    address_v1 = v1.ctypes.data
    address_u2 = u2.ctypes.data
    address_v2 = v2.ctypes.data
    address_w = w.ctypes.data
    address_x = x.ctypes.data
    address_y = y.ctypes.data
    address_z = z.ctypes.data
    for iteration in range(32):
        np.copyto(A, original_A)
        assert A.ctypes.data == address_A
        np.copyto(u1, original_u1)
        assert u1.ctypes.data == address_u1
        np.copyto(v1, original_v1)
        assert v1.ctypes.data == address_v1
        np.copyto(u2, original_u2)
        assert u2.ctypes.data == address_u2
        np.copyto(v2, original_v2)
        assert v2.ctypes.data == address_v2
        np.copyto(w, original_w)
        assert w.ctypes.data == address_w
        np.copyto(x, original_x)
        assert x.ctypes.data == address_x
        np.copyto(y, original_y)
        assert y.ctypes.data == address_y
        np.copyto(z, original_z)
        assert z.ctypes.data == address_z
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(A, original_A)
        np.copyto(u1, original_u1)
        np.copyto(v1, original_v1)
        np.copyto(u2, original_u2)
        np.copyto(v2, original_v2)
        np.copyto(w, original_w)
        np.copyto(x, original_x)
        np.copyto(y, original_y)
        np.copyto(z, original_z)
        elapsed = sample(alpha, beta, A, u1, v1, u2, v2, w, x, y, z)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
