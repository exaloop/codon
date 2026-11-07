# FANNKUCH benchmark
from math import factorial as fact
from sys import argv
from time import time

def perm(n, i):
    p = [0] * n

    for k in range(n):
        f = fact(n - 1 - k)
        p[k] = i // f
        i = i % f

    for k in range(n - 1, -1, -1):
        for j in range(k - 1, -1, -1):
            if p[j] <= p[k]:
                p[k] += 1

    return p

n = int(argv[1])
max_flips = 0

t0 = time()
for idx in range(fact(n)):
    p = perm(n, idx)
    flips = 0
    k = p[0]

    while k:
        i = 0
        j = k
        while i < j:
            p[i], p[j] = p[j], p[i]
            i += 1
            j -= 1

        k = p[0]
        flips += 1

    max_flips = max(flips, max_flips)

print(f'Pfannkuchen({n}) = {max_flips}')
t1 = time()
print(t1 - t0)
