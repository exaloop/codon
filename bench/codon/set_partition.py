# https://stackoverflow.com/questions/73473074/speed-up-set-partition-generation-by-skipping-ones-with-subsets-smaller-or-large
import sys
import time

def conforms(candidate, minsize, forgive):
    """
    Check if partition `candidate` is at most `forgive` additions from making
    all its elements conform to having minimum size `minsize`
    """
    deficit = 0
    for p in candidate:
        need = minsize - len(p)
        if need > 0:
            deficit += need

    # Is the deficit small enough?
    return (deficit <= forgive)

def partition_filtered(collection, minsize=1, forgive=0):
    """
    Generate partitions that contain at least `minsize` elements per set;
    allow `forgive` missing elements, which can get added in subsequent steps
    """
    if len(collection) == 1:
        yield [ collection ]
        return

    first = collection[0]
    for smaller in partition_filtered(collection[1:], minsize, forgive=forgive+1):
        # insert `first` in each of the subpartition's subsets
        for n, subset in enumerate(smaller):
            candidate = smaller[:n] + [[ first ] + subset]  + smaller[n+1:]
            if conforms(candidate, minsize, forgive):
                yield candidate

        # put `first` in its own subset
        candidate = [ [ first ] ] + smaller
        if conforms(candidate, minsize, forgive):
            yield candidate


import time

t = time.time()
something = list(range(1, int(sys.argv[1])))
v = partition_filtered(something, minsize=2)
x = 0
for p in v:
    p.sort()
    x += p[len(p) // 3][0]
print(x)
print(time.time() - t)
