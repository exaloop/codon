import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/covariance/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
M = 3200
float_n = 4000.0
original_data = np.load(directory + '/input-data.npy')
data = np.empty_like(original_data)
np.copyto(data, original_data)
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

def sample(M, float_n, data):
    started = time.perf_counter()
    result = kernel_module.kernel(M, float_n, data)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_data = data.ctypes.data
    for iteration in range(32):
        np.copyto(data, original_data)
        assert data.ctypes.data == address_data
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(M, float_n, data)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(data, original_data)
        elapsed = sample(M, float_n, data)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
