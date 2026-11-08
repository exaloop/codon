import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/spmv/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
original_A_row = np.load(directory + '/input-A_row.npy')
A_row = np.empty_like(original_A_row)
np.copyto(A_row, original_A_row)
original_A_col = np.load(directory + '/input-A_col.npy')
A_col = np.empty_like(original_A_col)
np.copyto(A_col, original_A_col)
original_A_val = np.load(directory + '/input-A_val.npy')
A_val = np.empty_like(original_A_val)
np.copyto(A_val, original_A_val)
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

def sample(A_row, A_col, A_val, x):
    started = time.perf_counter()
    result = kernel_module.spmv(A_row, A_col, A_val, x)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_A_row = A_row.ctypes.data
    address_A_col = A_col.ctypes.data
    address_A_val = A_val.ctypes.data
    address_x = x.ctypes.data
    for iteration in range(32):
        np.copyto(A_row, original_A_row)
        assert A_row.ctypes.data == address_A_row
        np.copyto(A_col, original_A_col)
        assert A_col.ctypes.data == address_A_col
        np.copyto(A_val, original_A_val)
        assert A_val.ctypes.data == address_A_val
        np.copyto(x, original_x)
        assert x.ctypes.data == address_x
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(A_row, A_col, A_val, x)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(A_row, original_A_row)
        np.copyto(A_col, original_A_col)
        np.copyto(A_val, original_A_val)
        np.copyto(x, original_x)
        elapsed = sample(A_row, A_col, A_val, x)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
