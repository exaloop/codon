import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/nbody/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
original_mass = np.load(directory + '/input-mass.npy')
mass = np.empty_like(original_mass)
np.copyto(mass, original_mass)
original_pos = np.load(directory + '/input-pos.npy')
pos = np.empty_like(original_pos)
np.copyto(pos, original_pos)
original_vel = np.load(directory + '/input-vel.npy')
vel = np.empty_like(original_vel)
np.copyto(vel, original_vel)
N = 100
Nt = 900
dt = 0.01
G = 1.0
softening = 0.1

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(mass, pos, vel, N, Nt, dt, G, softening):
    started = time.perf_counter()
    result = kernel_module.nbody(mass, pos, vel, N, Nt, dt, G, softening)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_mass = mass.ctypes.data
    address_pos = pos.ctypes.data
    address_vel = vel.ctypes.data
    for iteration in range(32):
        np.copyto(mass, original_mass)
        assert mass.ctypes.data == address_mass
        np.copyto(pos, original_pos)
        assert pos.ctypes.data == address_pos
        np.copyto(vel, original_vel)
        assert vel.ctypes.data == address_vel
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(mass, pos, vel, N, Nt, dt, G, softening)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(mass, original_mass)
        np.copyto(pos, original_pos)
        np.copyto(vel, original_vel)
        elapsed = sample(mass, pos, vel, N, Nt, dt, G, softening)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
