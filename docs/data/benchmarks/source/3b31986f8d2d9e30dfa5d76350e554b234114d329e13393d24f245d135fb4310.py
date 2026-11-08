from sys import argv
from time import time

def is_prime(n):
    factors = 0
    for i in range(2, n):
        if n % i == 0:
            factors += 1
    return factors == 0

limit = int(argv[1])
total = 0

t0 = time()
for i in range(2, limit):
    if is_prime(i):
        total += 1
t1 = time()

print(total)
print(t1 - t0)
