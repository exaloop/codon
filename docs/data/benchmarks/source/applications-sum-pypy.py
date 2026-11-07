# https://towardsdatascience.com/getting-started-with-pypy-ef4ba5cb431c
import time
t1 = time.perf_counter()
nums = range(50000000)
sum = 0
for k in nums:
    sum = sum + k
print("Sum of 50000000 numbers is : ", sum)
t2 = time.perf_counter()
t = t2 - t1
print(t)
