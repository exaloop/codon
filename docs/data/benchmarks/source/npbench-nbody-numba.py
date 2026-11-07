# Adapted from https://github.com/pmocz/nbody-python/blob/master/nbody.py
# TODO: Add GPL-3.0 License

import numpy as np
import numba as nb
"""
Create Your Own N-body Simulation (With Python)
Philip Mocz (2020) Princeton Univeristy, @PMocz
Simulate orbits of stars interacting due to gravity
Code calculates pairwise forces according to Newton's Law of Gravity
"""


@nb.jit(nopython=True, parallel=False, fastmath=True)
def getAcc(pos, mass, G, softening):
    """
    Calculate the acceleration on each particle due to Newton's Law 
    pos  is an N x 3 matrix of positions
    mass is an N x 1 vector of masses
    G is Newton's Gravitational constant
    softening is the softening length
    a is N x 3 matrix of accelerations
    """
    # positions r = [x,y,z] for all particles
    x = pos[:, 0:1]
    y = pos[:, 1:2]
    z = pos[:, 2:3]

    # matrix that stores all pairwise particle separations: r_j - r_i
    dx = x.T - x
    dy = y.T - y
    dz = z.T - z

    # matrix that stores 1/r^3 for all particle pairwise particle separations
    inv_r3 = (dx**2 + dy**2 + dz**2 + softening**2)
    # inv_r3[inv_r3>0] = inv_r3[inv_r3>0]**(-1.5)
    I = inv_r3 > 0
    for i in range(I.shape[0]):
        for j in range(I.shape[1]):
            if I[i, j]:
                inv_r3[i, j] **= -1.5

    ax = G * (dx * inv_r3) @ mass
    ay = G * (dy * inv_r3) @ mass
    az = G * (dz * inv_r3) @ mass

    # pack together the acceleration components
    a = np.hstack((ax, ay, az))

    return a


@nb.jit(nopython=True, parallel=False, fastmath=True)
def getEnergy(pos, vel, mass, G):
    """
    Get kinetic energy (KE) and potential energy (PE) of simulation
    pos is N x 3 matrix of positions
    vel is N x 3 matrix of velocities
    mass is an N x 1 vector of masses
    G is Newton's Gravitational constant
    KE is the kinetic energy of the system
    PE is the potential energy of the system
    """
    # Kinetic Energy:
    # KE = 0.5 * np.sum(np.sum( mass * vel**2 ))
    KE = 0.5 * np.sum(mass * vel**2)

    # Potential Energy:

    # positions r = [x,y,z] for all particles
    x = pos[:, 0:1]
    y = pos[:, 1:2]
    z = pos[:, 2:3]

    # matrix that stores all pairwise particle separations: r_j - r_i
    dx = x.T - x
    dy = y.T - y
    dz = z.T - z

    # matrix that stores 1/r for all particle pairwise particle separations
    inv_r = np.sqrt(dx**2 + dy**2 + dz**2)
    # inv_r[inv_r>0] = 1.0/inv_r[inv_r>0]
    I = inv_r > 0
    for i in range(I.shape[0]):
        for j in range(I.shape[1]):
            if I[i, j]:
                inv_r[i, j] = 1.0 / inv_r[i, j]

    # sum over upper triangle, to count each interaction only once
    # PE = G * np.sum(np.sum(np.triu(-(mass*mass.T)*inv_r,1)))
    PE = G * np.sum(np.triu(-(mass * mass.T) * inv_r, 1))

    return KE, PE


@nb.jit(nopython=True, parallel=False, fastmath=True)
def nbody(mass, pos, vel, N, Nt, dt, G, softening):

    # Convert to Center-of-Mass frame
    # vel -= np.mean(mass * vel, axis=0) / np.mean(mass)
    vel -= (np.sum(mass * vel, axis=0) / vel.shape[0]) / np.mean(mass)

    # calculate initial gravitational accelerations
    acc = getAcc(pos, mass, G, softening)

    # calculate initial energy of system
    # KE = np.ndarray(Nt+1, dtype=np.float64)
    # PE = np.ndarray(Nt+1, dtype=np.float64)
    KE = np.empty(Nt + 1, dtype=np.float64)
    PE = np.empty(Nt + 1, dtype=np.float64)
    KE[0], PE[0] = getEnergy(pos, vel, mass, G)

    t = 0.0

    # Simulation Main Loop
    for i in range(Nt):
        # (1/2) kick
        vel += acc * dt / 2.0

        # drift
        pos += vel * dt

        # update accelerations
        acc = getAcc(pos, mass, G, softening)

        # (1/2) kick
        vel += acc * dt / 2.0

        # update time
        t += dt

        # get energy of system
        KE[i + 1], PE[i + 1] = getEnergy(pos, vel, mass, G)

    return KE, PE
