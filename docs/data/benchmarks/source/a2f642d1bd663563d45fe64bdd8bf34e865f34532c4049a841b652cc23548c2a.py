import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/gesummv/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
alpha = 1.5
beta = 1.2
original_A = np.load(directory + '/input-A.npy')
A = np.empty_like(original_A)
np.copyto(A, original_A)
original_B = np.load(directory + '/input-B.npy')
B = np.empty_like(original_B)
np.copyto(B, original_B)
original_x = np.load(directory + '/input-x.npy')
x = np.empty_like(original_x)
np.copyto(x, original_x)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(alpha, beta, A, B, x):
    started = time.perf_counter()
    result = kernel_module.kernel(alpha, beta, A, B, x)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_A = A.ctypes.data
    address_B = B.ctypes.data
    address_x = x.ctypes.data
    for iteration in range(32):
        np.copyto(A, original_A)
        assert A.ctypes.data == address_A
        np.copyto(B, original_B)
        assert B.ctypes.data == address_B
        np.copyto(x, original_x)
        assert x.ctypes.data == address_x
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(alpha, beta, A, B, x)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(A, original_A)
        np.copyto(B, original_B)
        np.copyto(x, original_x)
        elapsed = sample(alpha, beta, A, B, x)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
