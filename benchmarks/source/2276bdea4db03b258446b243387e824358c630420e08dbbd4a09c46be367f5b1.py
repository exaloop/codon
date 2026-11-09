import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/hdiff/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
original_in_field = np.load(directory + '/input-in_field.npy')
in_field = np.empty_like(original_in_field)
np.copyto(in_field, original_in_field)
original_out_field = np.load(directory + '/input-out_field.npy')
out_field = np.empty_like(original_out_field)
np.copyto(out_field, original_out_field)
original_coeff = np.load(directory + '/input-coeff.npy')
coeff = np.empty_like(original_coeff)
np.copyto(coeff, original_coeff)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(in_field, out_field, coeff):
    started = time.perf_counter()
    result = kernel_module.hdiff(in_field, out_field, coeff)
    elapsed = time.perf_counter() - started
    emit(out_field, directory + '/output-out_field')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_in_field = in_field.ctypes.data
    address_out_field = out_field.ctypes.data
    address_coeff = coeff.ctypes.data
    for iteration in range(32):
        np.copyto(in_field, original_in_field)
        assert in_field.ctypes.data == address_in_field
        np.copyto(out_field, original_out_field)
        assert out_field.ctypes.data == address_out_field
        np.copyto(coeff, original_coeff)
        assert coeff.ctypes.data == address_coeff
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(in_field, out_field, coeff)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(in_field, original_in_field)
        np.copyto(out_field, original_out_field)
        np.copyto(coeff, original_coeff)
        elapsed = sample(in_field, out_field, coeff)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
