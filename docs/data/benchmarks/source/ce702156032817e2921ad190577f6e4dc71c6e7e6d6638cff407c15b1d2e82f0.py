import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/trisolv/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
original_L = np.load(directory + '/input-L.npy')
L = np.empty_like(original_L)
np.copyto(L, original_L)
original_x = np.load(directory + '/input-x.npy')
x = np.empty_like(original_x)
np.copyto(x, original_x)
original_b = np.load(directory + '/input-b.npy')
b = np.empty_like(original_b)
np.copyto(b, original_b)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(L, x, b):
    started = time.perf_counter()
    result = kernel_module.kernel(L, x, b)
    elapsed = time.perf_counter() - started
    emit(b, directory + '/output-b')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_L = L.ctypes.data
    address_x = x.ctypes.data
    address_b = b.ctypes.data
    for iteration in range(32):
        np.copyto(L, original_L)
        assert L.ctypes.data == address_L
        np.copyto(x, original_x)
        assert x.ctypes.data == address_x
        np.copyto(b, original_b)
        assert b.ctypes.data == address_b
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(L, x, b)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(L, original_L)
        np.copyto(x, original_x)
        np.copyto(b, original_b)
        elapsed = sample(L, x, b)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
