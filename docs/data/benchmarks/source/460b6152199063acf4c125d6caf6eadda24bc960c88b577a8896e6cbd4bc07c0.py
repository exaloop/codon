import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/vadv/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
original_utens_stage = np.load(directory + '/input-utens_stage.npy')
utens_stage = np.empty_like(original_utens_stage)
np.copyto(utens_stage, original_utens_stage)
original_u_stage = np.load(directory + '/input-u_stage.npy')
u_stage = np.empty_like(original_u_stage)
np.copyto(u_stage, original_u_stage)
original_wcon = np.load(directory + '/input-wcon.npy')
wcon = np.empty_like(original_wcon)
np.copyto(wcon, original_wcon)
original_u_pos = np.load(directory + '/input-u_pos.npy')
u_pos = np.empty_like(original_u_pos)
np.copyto(u_pos, original_u_pos)
original_utens = np.load(directory + '/input-utens.npy')
utens = np.empty_like(original_utens)
np.copyto(utens, original_utens)
dtr_stage = 0.15
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

def sample(utens_stage, u_stage, wcon, u_pos, utens, dtr_stage):
    started = time.perf_counter()
    result = kernel_module.vadv(utens_stage, u_stage, wcon, u_pos, utens, dtr_stage)
    elapsed = time.perf_counter() - started
    emit(utens_stage, directory + '/output-utens_stage')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_utens_stage = utens_stage.ctypes.data
    address_u_stage = u_stage.ctypes.data
    address_wcon = wcon.ctypes.data
    address_u_pos = u_pos.ctypes.data
    address_utens = utens.ctypes.data
    for iteration in range(32):
        np.copyto(utens_stage, original_utens_stage)
        assert utens_stage.ctypes.data == address_utens_stage
        np.copyto(u_stage, original_u_stage)
        assert u_stage.ctypes.data == address_u_stage
        np.copyto(wcon, original_wcon)
        assert wcon.ctypes.data == address_wcon
        np.copyto(u_pos, original_u_pos)
        assert u_pos.ctypes.data == address_u_pos
        np.copyto(utens, original_utens)
        assert utens.ctypes.data == address_utens
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(utens_stage, original_utens_stage)
        np.copyto(u_stage, original_u_stage)
        np.copyto(wcon, original_wcon)
        np.copyto(u_pos, original_u_pos)
        np.copyto(utens, original_utens)
        elapsed = sample(utens_stage, u_stage, wcon, u_pos, utens, dtr_stage)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
