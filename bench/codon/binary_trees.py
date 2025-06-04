# The Computer Language Benchmarks Game
# http://benchmarksgame.alioth.debian.org/
#
# contributed by Antoine Pitrou
# modified by Dominique Wahli and Daniel Nanz
# modified by Joerg Baumann
# modified by @arshajii for Codon

import sys
import time

class Node:
    def __init__(self, left = None, right = None):
        self.left = left
        self.right = right

def make_tree(d):
    return Node(make_tree(d - 1), make_tree(d - 1)) if d > 0 else Node()

def check_tree(node):
    l, r = node.left, node.right
    if l is None:
        return 1
    else:
        return 1 + check_tree(l) + check_tree(r)

def make_check(itde, make=make_tree, check=check_tree):
    i, d = itde
    return check(make(d))

def get_argchunks(i, d, chunksize=5000):
    assert chunksize % 2 == 0
    chunk = []
    for k in range(1, i + 1):
        chunk.append((k, d))
        if len(chunk) == chunksize:
            yield chunk
            chunk = []
    if len(chunk) > 0:
        yield chunk

def main(n, min_depth=4):
    max_depth = max(min_depth + 2, n)
    stretch_depth = max_depth + 1
    print(f'stretch tree of depth {stretch_depth}\t check: {make_check((0, stretch_depth))}')

    long_lived_tree = make_tree(max_depth)

    mmd = max_depth + min_depth
    for d in range(min_depth, stretch_depth, 2):
        i = 2 ** (mmd - d)
        cs = 0
        for argchunk in get_argchunks(i, d):
            cs += sum(map(make_check, argchunk))
        print(f'{i}\t trees of depth {d}\t check: {cs}')

    print(f'long lived tree of depth {max_depth}\t check: {check_tree(long_lived_tree)}')

t0 = time.time()
main(int(sys.argv[1]))
t1 = time.time()
print(t1 - t0)
