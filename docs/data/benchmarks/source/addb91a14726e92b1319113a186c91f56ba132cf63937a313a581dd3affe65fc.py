import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/fdtd_2d/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
TMAX = 150
original_ex = np.load(directory + '/input-ex.npy')
ex = np.empty_like(original_ex)
np.copyto(ex, original_ex)
original_ey = np.load(directory + '/input-ey.npy')
ey = np.empty_like(original_ey)
np.copyto(ey, original_ey)
original_hz = np.load(directory + '/input-hz.npy')
hz = np.empty_like(original_hz)
np.copyto(hz, original_hz)
original__fict_ = np.load(directory + '/input-_fict_.npy')
_fict_ = np.empty_like(original__fict_)
np.copyto(_fict_, original__fict_)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(TMAX, ex, ey, hz, _fict_):
    started = time.perf_counter()
    result = kernel_module.kernel(TMAX, ex, ey, hz, _fict_)
    elapsed = time.perf_counter() - started
    emit(ex, directory + '/output-ex')
    emit(ey, directory + '/output-ey')
    emit(hz, directory + '/output-hz')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_ex = ex.ctypes.data
    address_ey = ey.ctypes.data
    address_hz = hz.ctypes.data
    address__fict_ = _fict_.ctypes.data
    for iteration in range(32):
        np.copyto(ex, original_ex)
        assert ex.ctypes.data == address_ex
        np.copyto(ey, original_ey)
        assert ey.ctypes.data == address_ey
        np.copyto(hz, original_hz)
        assert hz.ctypes.data == address_hz
        np.copyto(_fict_, original__fict_)
        assert _fict_.ctypes.data == address__fict_
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(TMAX, ex, ey, hz, _fict_)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(ex, original_ex)
        np.copyto(ey, original_ey)
        np.copyto(hz, original_hz)
        np.copyto(_fict_, original__fict_)
        elapsed = sample(TMAX, ex, ey, hz, _fict_)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
