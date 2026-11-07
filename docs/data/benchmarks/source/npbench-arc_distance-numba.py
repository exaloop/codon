# Copyright (c) 2019, Serge Guelton
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# 	Redistributions of source code must retain the above copyright notice, this
# 	list of conditions and the following disclaimer.

# 	Redistributions in binary form must reproduce the above copyright notice,
# 	this list of conditions and the following disclaimer in the documentation
# 	and/or other materials provided with the distribution.

# 	Neither the name of HPCProject, Serge Guelton nor the names of its
# 	contributors may be used to endorse or promote products derived from this
# 	software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
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
def arc_distance(theta_1, phi_1, theta_2, phi_2):
    """
    Calculates the pairwise arc distance between all points in vector a and b.
    """
    temp = np.sin((theta_2 - theta_1) /
                  2)**2 + np.cos(theta_1) * np.cos(theta_2) * np.sin(
                      (phi_2 - phi_1) / 2)**2
    distance_matrix = 2 * (np.arctan2(np.sqrt(temp), np.sqrt(1 - temp)))
    return distance_matrix
