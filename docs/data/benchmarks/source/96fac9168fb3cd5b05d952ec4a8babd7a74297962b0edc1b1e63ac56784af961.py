import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/contour_integral/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
NR = 600
NM = 1000
slab_per_bc = 2
original_Ham = np.load(directory + '/input-Ham.npy')
Ham = np.empty_like(original_Ham)
np.copyto(Ham, original_Ham)
original_int_pts = np.load(directory + '/input-int_pts.npy')
int_pts = np.empty_like(original_int_pts)
np.copyto(int_pts, original_int_pts)
original_Y = np.load(directory + '/input-Y.npy')
Y = np.empty_like(original_Y)
np.copyto(Y, original_Y)
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

def sample(NR, NM, slab_per_bc, Ham, int_pts, Y):
    started = time.perf_counter()
    result = kernel_module.contour_integral(NR, NM, slab_per_bc, Ham, int_pts, Y)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_Ham = Ham.ctypes.data
    address_int_pts = int_pts.ctypes.data
    address_Y = Y.ctypes.data
    for iteration in range(32):
        np.copyto(Ham, original_Ham)
        assert Ham.ctypes.data == address_Ham
        np.copyto(int_pts, original_int_pts)
        assert int_pts.ctypes.data == address_int_pts
        np.copyto(Y, original_Y)
        assert Y.ctypes.data == address_Y
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(Ham, original_Ham)
        np.copyto(int_pts, original_int_pts)
        np.copyto(Y, original_Y)
        elapsed = sample(NR, NM, slab_per_bc, Ham, int_pts, Y)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
