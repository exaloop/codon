import numpy as np
import sys
import time
import gc
import importlib.util
import resource
import json
from threadpoolctl import threadpool_info, threadpool_limits
thread_limits = threadpool_limits(limits=1)
spec = importlib.util.spec_from_file_location('kernel_module', '$BENCH_WORK/npbench-final/mandelbrot1/python/kernel.py')
kernel_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kernel_module)
directory = sys.argv[1]
samples = int(sys.argv[2])
xmin = -2.0
xmax = 0.5
ymin = -1.25
ymax = 1.25
XN = 833
YN = 833
maxiter = 200
horizon = 2.0

def emit(value, filename):
    if value is None:
        return
    if isinstance(value, tuple):
        for index, item in enumerate(value):
            emit(item, filename + '_' + str(index))
    else:
        np.save(filename, np.asarray(value))

def sample(xmin, xmax, ymin, ymax, XN, YN, maxiter, horizon):
    started = time.perf_counter()
    result = kernel_module.mandelbrot(xmin, xmax, ymin, ymax, XN, YN, maxiter, horizon)
    elapsed = time.perf_counter() - started
    emit(result, directory + '/output-return')
    return elapsed

if samples == 0:
    gc.collect()
    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    for iteration in range(32):
        pass
    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before
    print('BENCH_MEMORY_RSS', rss_growth)
else:
    for iteration in range(samples + 1):
        gc.collect()
        elapsed = sample(xmin, xmax, ymin, ymax, XN, YN, maxiter, horizon)
        if iteration > 0:
            print('BENCH_TIME', elapsed)
    print('BENCH_RUNTIME', json.dumps(threadpool_info()))
