import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/go_fast/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
original_a = np.load(directory + '/input-a.npy')
a = np.empty_like(original_a)
np.copyto(a, original_a)
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

def sample(a):
    started = time.perf_counter()
    result = kernel_module.go_fast(a)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_a = a.ctypes.data
    for iteration in range(32):
        np.copyto(a, original_a)
        assert a.ctypes.data == address_a
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(a)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(a, original_a)
        elapsed = sample(a)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
