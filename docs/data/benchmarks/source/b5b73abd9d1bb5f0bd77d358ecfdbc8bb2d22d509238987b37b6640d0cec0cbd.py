import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/mvt/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
original_x1 = np.load(directory + '/input-x1.npy')
x1 = np.empty_like(original_x1)
np.copyto(x1, original_x1)
original_x2 = np.load(directory + '/input-x2.npy')
x2 = np.empty_like(original_x2)
np.copyto(x2, original_x2)
original_y_1 = np.load(directory + '/input-y_1.npy')
y_1 = np.empty_like(original_y_1)
np.copyto(y_1, original_y_1)
original_y_2 = np.load(directory + '/input-y_2.npy')
y_2 = np.empty_like(original_y_2)
np.copyto(y_2, original_y_2)
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

def sample(x1, x2, y_1, y_2, A):
    started = time.perf_counter()
    result = kernel_module.kernel(x1, x2, y_1, y_2, A)
    elapsed = time.perf_counter() - started
    emit(x1, directory + '/output-x1')
    emit(x2, directory + '/output-x2')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_x1 = x1.ctypes.data
    address_x2 = x2.ctypes.data
    address_y_1 = y_1.ctypes.data
    address_y_2 = y_2.ctypes.data
    address_A = A.ctypes.data
    for iteration in range(32):
        np.copyto(x1, original_x1)
        assert x1.ctypes.data == address_x1
        np.copyto(x2, original_x2)
        assert x2.ctypes.data == address_x2
        np.copyto(y_1, original_y_1)
        assert y_1.ctypes.data == address_y_1
        np.copyto(y_2, original_y_2)
        assert y_2.ctypes.data == address_y_2
        np.copyto(A, original_A)
        assert A.ctypes.data == address_A
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(x1, x2, y_1, y_2, A)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(x1, original_x1)
        np.copyto(x2, original_x2)
        np.copyto(y_1, original_y_1)
        np.copyto(y_2, original_y_2)
        np.copyto(A, original_A)
        elapsed = sample(x1, x2, y_1, y_2, A)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
