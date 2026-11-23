# Barba, Lorena A., and Forsyth, Gilbert F. (2018).
# CFD Python: the 12 steps to Navier-Stokes equations.
# Journal of Open Source Education, 1(9), 21,
# https://doi.org/10.21105/jose.00021
# TODO: License
# (c) 2017 Lorena A. Barba, Gilbert F. Forsyth.
# All content is under Creative Commons Attribution CC-BY 4.0,
# and all code is under BSD-3 clause (previously under MIT, and changed on March 8, 2018).

import numpy as np


def build_up_b(rho, dt, dx, dy, u, v):
    b = np.zeros_like(u)
    b[1:-1,
      1:-1] = (rho * (1 / dt * ((u[1:-1, 2:] - u[1:-1, 0:-2]) / (2 * dx) +
                                (v[2:, 1:-1] - v[0:-2, 1:-1]) / (2 * dy)) -
                      ((u[1:-1, 2:] - u[1:-1, 0:-2]) / (2 * dx))**2 - 2 *
                      ((u[2:, 1:-1] - u[0:-2, 1:-1]) / (2 * dy) *
                       (v[1:-1, 2:] - v[1:-1, 0:-2]) / (2 * dx)) -
                      ((v[2:, 1:-1] - v[0:-2, 1:-1]) / (2 * dy))**2))

    # Periodic BC Pressure @ x = 2
    b[1:-1, -1] = (rho * (1 / dt * ((u[1:-1, 0] - u[1:-1, -2]) / (2 * dx) +
                                    (v[2:, -1] - v[0:-2, -1]) / (2 * dy)) -
                          ((u[1:-1, 0] - u[1:-1, -2]) / (2 * dx))**2 - 2 *
                          ((u[2:, -1] - u[0:-2, -1]) / (2 * dy) *
                           (v[1:-1, 0] - v[1:-1, -2]) / (2 * dx)) -
                          ((v[2:, -1] - v[0:-2, -1]) / (2 * dy))**2))

    # Periodic BC Pressure @ x = 0
    b[1:-1, 0] = (rho * (1 / dt * ((u[1:-1, 1] - u[1:-1, -1]) / (2 * dx) +
                                   (v[2:, 0] - v[0:-2, 0]) / (2 * dy)) -
                         ((u[1:-1, 1] - u[1:-1, -1]) / (2 * dx))**2 - 2 *
                         ((u[2:, 0] - u[0:-2, 0]) / (2 * dy) *
                          (v[1:-1, 1] - v[1:-1, -1]) /
                          (2 * dx)) - ((v[2:, 0] - v[0:-2, 0]) / (2 * dy))**2))

    return b


def pressure_poisson_periodic(nit, p, dx, dy, b):
    pn = np.empty_like(p)

    for q in range(nit):
        pn = p.copy()
        p[1:-1, 1:-1] = (((pn[1:-1, 2:] + pn[1:-1, 0:-2]) * dy**2 +
                          (pn[2:, 1:-1] + pn[0:-2, 1:-1]) * dx**2) /
                         (2 * (dx**2 + dy**2)) - dx**2 * dy**2 /
                         (2 * (dx**2 + dy**2)) * b[1:-1, 1:-1])

        # Periodic BC Pressure @ x = 2
        p[1:-1, -1] = (((pn[1:-1, 0] + pn[1:-1, -2]) * dy**2 +
                        (pn[2:, -1] + pn[0:-2, -1]) * dx**2) /
                       (2 * (dx**2 + dy**2)) - dx**2 * dy**2 /
                       (2 * (dx**2 + dy**2)) * b[1:-1, -1])

        # Periodic BC Pressure @ x = 0
        p[1:-1,
          0] = (((pn[1:-1, 1] + pn[1:-1, -1]) * dy**2 +
                 (pn[2:, 0] + pn[0:-2, 0]) * dx**2) / (2 * (dx**2 + dy**2)) -
                dx**2 * dy**2 / (2 * (dx**2 + dy**2)) * b[1:-1, 0])

        # Wall boundary conditions, pressure
        p[-1, :] = p[-2, :]  # dp/dy = 0 at y = 2
        p[0, :] = p[1, :]  # dp/dy = 0 at y = 0


def channel_flow(nit, u, v, dt, dx, dy, p, rho, nu, F):
    udiff = 1
    stepcount = 0

    while udiff > .001:
        un = u.copy()
        vn = v.copy()

        b = build_up_b(rho, dt, dx, dy, u, v)
        pressure_poisson_periodic(nit, p, dx, dy, b)

        u[1:-1,
          1:-1] = (un[1:-1, 1:-1] - un[1:-1, 1:-1] * dt / dx *
                   (un[1:-1, 1:-1] - un[1:-1, 0:-2]) -
                   vn[1:-1, 1:-1] * dt / dy *
                   (un[1:-1, 1:-1] - un[0:-2, 1:-1]) - dt / (2 * rho * dx) *
                   (p[1:-1, 2:] - p[1:-1, 0:-2]) + nu *
                   (dt / dx**2 *
                    (un[1:-1, 2:] - 2 * un[1:-1, 1:-1] + un[1:-1, 0:-2]) +
                    dt / dy**2 *
                    (un[2:, 1:-1] - 2 * un[1:-1, 1:-1] + un[0:-2, 1:-1])) +
                   F * dt)

        v[1:-1,
          1:-1] = (vn[1:-1, 1:-1] - un[1:-1, 1:-1] * dt / dx *
                   (vn[1:-1, 1:-1] - vn[1:-1, 0:-2]) -
                   vn[1:-1, 1:-1] * dt / dy *
                   (vn[1:-1, 1:-1] - vn[0:-2, 1:-1]) - dt / (2 * rho * dy) *
                   (p[2:, 1:-1] - p[0:-2, 1:-1]) + nu *
                   (dt / dx**2 *
                    (vn[1:-1, 2:] - 2 * vn[1:-1, 1:-1] + vn[1:-1, 0:-2]) +
                    dt / dy**2 *
                    (vn[2:, 1:-1] - 2 * vn[1:-1, 1:-1] + vn[0:-2, 1:-1])))

        # Periodic BC u @ x = 2
        u[1:-1, -1] = (
            un[1:-1, -1] - un[1:-1, -1] * dt / dx *
            (un[1:-1, -1] - un[1:-1, -2]) - vn[1:-1, -1] * dt / dy *
            (un[1:-1, -1] - un[0:-2, -1]) - dt / (2 * rho * dx) *
            (p[1:-1, 0] - p[1:-1, -2]) + nu *
            (dt / dx**2 *
             (un[1:-1, 0] - 2 * un[1:-1, -1] + un[1:-1, -2]) + dt / dy**2 *
             (un[2:, -1] - 2 * un[1:-1, -1] + un[0:-2, -1])) + F * dt)

        # Periodic BC u @ x = 0
        u[1:-1,
          0] = (un[1:-1, 0] - un[1:-1, 0] * dt / dx *
                (un[1:-1, 0] - un[1:-1, -1]) - vn[1:-1, 0] * dt / dy *
                (un[1:-1, 0] - un[0:-2, 0]) - dt / (2 * rho * dx) *
                (p[1:-1, 1] - p[1:-1, -1]) + nu *
                (dt / dx**2 *
                 (un[1:-1, 1] - 2 * un[1:-1, 0] + un[1:-1, -1]) + dt / dy**2 *
                 (un[2:, 0] - 2 * un[1:-1, 0] + un[0:-2, 0])) + F * dt)

        # Periodic BC v @ x = 2
        v[1:-1, -1] = (
            vn[1:-1, -1] - un[1:-1, -1] * dt / dx *
            (vn[1:-1, -1] - vn[1:-1, -2]) - vn[1:-1, -1] * dt / dy *
            (vn[1:-1, -1] - vn[0:-2, -1]) - dt / (2 * rho * dy) *
            (p[2:, -1] - p[0:-2, -1]) + nu *
            (dt / dx**2 *
             (vn[1:-1, 0] - 2 * vn[1:-1, -1] + vn[1:-1, -2]) + dt / dy**2 *
             (vn[2:, -1] - 2 * vn[1:-1, -1] + vn[0:-2, -1])))

        # Periodic BC v @ x = 0
        v[1:-1,
          0] = (vn[1:-1, 0] - un[1:-1, 0] * dt / dx *
                (vn[1:-1, 0] - vn[1:-1, -1]) - vn[1:-1, 0] * dt / dy *
                (vn[1:-1, 0] - vn[0:-2, 0]) - dt / (2 * rho * dy) *
                (p[2:, 0] - p[0:-2, 0]) + nu *
                (dt / dx**2 *
                 (vn[1:-1, 1] - 2 * vn[1:-1, 0] + vn[1:-1, -1]) + dt / dy**2 *
                 (vn[2:, 0] - 2 * vn[1:-1, 0] + vn[0:-2, 0])))

        # Wall BC: u,v = 0 @ y = 0,2
        u[0, :] = 0
        u[-1, :] = 0
        v[0, :] = 0
        v[-1, :] = 0

        udiff = (np.sum(u) - np.sum(un)) / np.sum(u)
        stepcount += 1

    return stepcount
