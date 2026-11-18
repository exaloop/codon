import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/arc_distance/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
original_theta_1 = np.load(directory + '/input-theta_1.npy')
theta_1 = np.empty_like(original_theta_1)
np.copyto(theta_1, original_theta_1)
original_phi_1 = np.load(directory + '/input-phi_1.npy')
phi_1 = np.empty_like(original_phi_1)
np.copyto(phi_1, original_phi_1)
original_theta_2 = np.load(directory + '/input-theta_2.npy')
theta_2 = np.empty_like(original_theta_2)
np.copyto(theta_2, original_theta_2)
original_phi_2 = np.load(directory + '/input-phi_2.npy')
phi_2 = np.empty_like(original_phi_2)
np.copyto(phi_2, original_phi_2)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(theta_1, phi_1, theta_2, phi_2):
    started = time.perf_counter()
    result = kernel_module.arc_distance(theta_1, phi_1, theta_2, phi_2)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_theta_1 = theta_1.ctypes.data
    address_phi_1 = phi_1.ctypes.data
    address_theta_2 = theta_2.ctypes.data
    address_phi_2 = phi_2.ctypes.data
    for iteration in range(32):
        np.copyto(theta_1, original_theta_1)
        assert theta_1.ctypes.data == address_theta_1
        np.copyto(phi_1, original_phi_1)
        assert phi_1.ctypes.data == address_phi_1
        np.copyto(theta_2, original_theta_2)
        assert theta_2.ctypes.data == address_theta_2
        np.copyto(phi_2, original_phi_2)
        assert phi_2.ctypes.data == address_phi_2
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(theta_1, phi_1, theta_2, phi_2)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(theta_1, original_theta_1)
        np.copyto(phi_1, original_phi_1)
        np.copyto(theta_2, original_theta_2)
        np.copyto(phi_2, original_phi_2)
        elapsed = sample(theta_1, phi_1, theta_2, phi_2)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
