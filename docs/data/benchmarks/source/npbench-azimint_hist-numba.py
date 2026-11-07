# Copyright 2014 Jérôme Kieffer et al.
# This is an open-access article distributed under the terms of the
# Creative Commons Attribution License, which permits unrestricted use,
# distribution, and reproduction in any medium, provided the original author
# and source are credited.
# http://creativecommons.org/licenses/by/3.0/
# Jérôme Kieffer and Giannis Ashiotis. Pyfai: a python library for
# high performance azimuthal integration on gpu, 2014. In Proceedings of the
# 7th European Conference on Python in Science (EuroSciPy 2014).

# BSD 2-Clause License

# Copyright (c) 2017, Numba
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.

# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import numpy as np
import numba as nb


@nb.jit(nopython=True, parallel=False, fastmath=True)
def get_bin_edges(a, bins):
    bin_edges = np.zeros((bins + 1, ), dtype=np.float64)
    a_min = a.min()
    a_max = a.max()
    delta = (a_max - a_min) / bins
    for i in range(bin_edges.shape[0]):
        bin_edges[i] = a_min + i * delta

    bin_edges[-1] = a_max  # Avoid roundoff error on last point
    return bin_edges


@nb.jit(nopython=True, fastmath=True)
def compute_bin(x, bin_edges):
    # assuming uniform bins for now
    n = bin_edges.shape[0] - 1
    a_min = bin_edges[0]
    a_max = bin_edges[-1]

    # special case to mirror NumPy behavior for last bin
    if x == a_max:
        return n - 1  # a_max always in last bin

    return int(n * (x - a_min) / (a_max - a_min))


@nb.jit(nopython=True, parallel=False, fastmath=True)
def histogram(a, bins, weights):
    hist = np.zeros((bins, ), dtype=a.dtype)
    bin_edges = get_bin_edges(a, bins)

    for i in range(a.shape[0]):
        bin = compute_bin(a[i], bin_edges)
        hist[bin] += weights[i]

    return hist, bin_edges


@nb.jit(nopython=True, parallel=False, fastmath=True)
def azimint_hist(data, radius, npt):
    histu = np.histogram(radius, npt)[0]
    # histw = np.histogram(radius, npt, weights=data)[0]
    histw = histogram(radius, npt, weights=data)[0]
    return histw / histu
