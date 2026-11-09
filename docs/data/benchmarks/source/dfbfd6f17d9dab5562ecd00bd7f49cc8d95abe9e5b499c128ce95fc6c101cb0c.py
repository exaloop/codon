import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/scattering_self_energies/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
original_neigh_idx = np.load(directory + '/input-neigh_idx.npy')
neigh_idx = np.empty_like(original_neigh_idx)
np.copyto(neigh_idx, original_neigh_idx)
original_dH = np.load(directory + '/input-dH.npy')
dH = np.empty_like(original_dH)
np.copyto(dH, original_dH)
original_G = np.load(directory + '/input-G.npy')
G = np.empty_like(original_G)
np.copyto(G, original_G)
original_D = np.load(directory + '/input-D.npy')
D = np.empty_like(original_D)
np.copyto(D, original_D)
original_Sigma = np.load(directory + '/input-Sigma.npy')
Sigma = np.empty_like(original_Sigma)
np.copyto(Sigma, original_Sigma)

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(neigh_idx, dH, G, D, Sigma):
    started = time.perf_counter()
    result = kernel_module.scattering_self_energies(neigh_idx, dH, G, D, Sigma)
    elapsed = time.perf_counter() - started
    emit(Sigma, directory + '/output-Sigma')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    address_neigh_idx = neigh_idx.ctypes.data
    address_dH = dH.ctypes.data
    address_G = G.ctypes.data
    address_D = D.ctypes.data
    address_Sigma = Sigma.ctypes.data
    for iteration in range(32):
        np.copyto(neigh_idx, original_neigh_idx)
        assert neigh_idx.ctypes.data == address_neigh_idx
        np.copyto(dH, original_dH)
        assert dH.ctypes.data == address_dH
        np.copyto(G, original_G)
        assert G.ctypes.data == address_G
        np.copyto(D, original_D)
        assert D.ctypes.data == address_D
        np.copyto(Sigma, original_Sigma)
        assert Sigma.ctypes.data == address_Sigma
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        np.copyto(neigh_idx, original_neigh_idx)
        np.copyto(dH, original_dH)
        np.copyto(G, original_G)
        np.copyto(D, original_D)
        np.copyto(Sigma, original_Sigma)
        elapsed = sample(neigh_idx, dH, G, D, Sigma)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
