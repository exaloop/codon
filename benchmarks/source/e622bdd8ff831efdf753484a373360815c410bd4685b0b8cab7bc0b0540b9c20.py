import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/numba-no-fastmath-L-20261108-seeded/cavity_flow/numba/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
from numba.core.registry import CPUDispatcher
dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}
assert dispatchers, 'No Numba dispatchers found'
assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'
print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))
directory = sys.argv[1]
samples = int(sys.argv[2])
nx = 201
ny = 201
nt = 100
nit = 20
original_u = np.load(directory + '/input-u.npy')
u = np.empty_like(original_u)
np.copyto(u, original_u)
original_v = np.load(directory + '/input-v.npy')
v = np.empty_like(original_v)
np.copyto(v, original_v)
dt = 2.5e-06
dx = 0.01
dy = 0.01
original_p = np.load(directory + '/input-p.npy')
p = np.empty_like(original_p)
np.copyto(p, original_p)
rho = 1.0
nu = 0.1

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(nx, ny, nt, nit, u, v, dt, dx, dy, p, rho, nu):
    started = time.perf_counter()
    result = kernel_module.cavity_flow(nx, ny, nt, nit, u, v, dt, dx, dy, p, rho, nu)
    elapsed = time.perf_counter() - started
    emit(u, directory + '/output-u')
    emit(v, directory + '/output-v')
    emit(p, directory + '/output-p')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_u = u.ctypes.data
    address_v = v.ctypes.data
    address_p = p.ctypes.data
    for iteration in range(32):
        np.copyto(u, original_u)
        assert u.ctypes.data == address_u
        np.copyto(v, original_v)
        assert v.ctypes.data == address_v
        np.copyto(p, original_p)
        assert p.ctypes.data == address_p
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
elif samples == -1:
    sample(nx, ny, nt, nit, u, v, dt, dx, dy, p, rho, nu)
    print('BENCH_REFERENCE_ONLY')
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(u, original_u)
        np.copyto(v, original_v)
        np.copyto(p, original_p)
        elapsed = sample(nx, ny, nt, nit, u, v, dt, dx, dy, p, rho, nu)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
