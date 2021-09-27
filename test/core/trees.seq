import sys

def max(a, b):
    return a if a>=b else b

class AVLNode[K,V]:
    key: K
    value: V
    parent: Optional[AVLNode[K,V]]
    left: Optional[AVLNode[K,V]]
    right: Optional[AVLNode[K,V]]
    height: int

    def __init__(self: AVLNode[K,V], parent: Optional[AVLNode[K,V]], k: K, v: V):
        self.key = k
        self.value = v
        self.parent = parent
        self.left = None
        self.right = None
        self.height = -1

    def find(self, k):
        if k < self.key:
            if self.left is None:
                return None
            else:
                return self.left.find(k)
        elif k == self.key:
            return self
        else:
            if self.right is None:
                return None
            else:
                return self.right.find(k)

    def find_min(self):
        current: Optional[AVLNode[K, V]] = self
        while current.left is not None:
            current = current.left
        return current

    def next_larger(self):
        if self.right is not None:
            return self.right.find_min()
        current: Optional[AVLNode[K, V]] = self
        while current.parent is not None and current is current.parent.right:
            current = current.parent
        return current.parent

    def insert(self, node):
        if node is None:
            return
        if node.key < self.key:
            if self.left is None:
                node.parent = self
                self.left = node
            else:
                self.left.insert(node)
        else:
            if self.right is None:
                node.parent = self
                self.right = node
            else:
                self.right.insert(node)

    def delete(self):
        if self.left is None or self.right is None:
            if self is self.parent.left:
                self.parent.left = self.left if self.left else self.right
                if self.parent.left is not None:
                    self.parent.left.parent = self.parent
            else:
                self.parent.right = self.left if self.left else self.right
                if self.parent.right is not None:
                    self.parent.right.parent = self.parent
            return self
        else:
            s = self.next_larger()
            self.key, s.key = s.key, self.key
            return s.delete()

    def __iter__(self: AVLNode[K,V]):
        if self.left:
            for i in self.left:
                yield i
        yield self
        if self.right:
            for i in self.right:
                yield i

def height(node):
    if node is None:
        return -1
    else:
        return node.height

def update_height(node):
    node.height = max(height(node.left), height(node.right)) + 1

class AVL[K,V]:
    root: Optional[AVLNode[K,V]]

    def __init__(self: AVL[K,V]):
        self.root = None

    def find(self, k):
        if not self.root:
            return None
        return self.root.find(k)

    def find_min(self):
        if not self.root:
            return None
        return self.root.find_min()

    def next_larger(self, k):
        node = self.find(k)
        return node.next_larger() if node else None

    def left_rotate(self, x):
        y = x.right
        y.parent = x.parent
        if y.parent is None:
            self.root = y
        else:
            if y.parent.left is x:
                y.parent.left = y
            elif y.parent.right is x:
                y.parent.right = y
        x.right = y.left
        if x.right is not None:
            x.right.parent = x
        y.left = x
        x.parent = y
        update_height(x)
        update_height(y)

    def right_rotate(self, x):
        y = x.left
        y.parent = x.parent
        if y.parent is None:
            self.root = y
        else:
            if y.parent.left is x:
                y.parent.left = y
            elif y.parent.right is x:
                y.parent.right = y
        x.left = y.right
        if x.left is not None:
            x.left.parent = x
        y.right = x
        x.parent = y
        update_height(x)
        update_height(y)

    def rebalance(self, node: Optional[AVLNode[K, V]]):
        while node is not None:
            update_height(node)
            if height(node.left) >= 2 + height(node.right):
                if height(node.left.left) >= height(node.left.right):
                    self.right_rotate(node)
                else:
                    self.left_rotate(node.left)
                    self.right_rotate(node)
            elif height(node.right) >= 2 + height(node.left):
                if height(node.right.right) >= height(node.right.left):
                    self.left_rotate(node)
                else:
                    self.right_rotate(node.right)
                    self.left_rotate(node)
            node = node.parent

    def insert(self, k, v):
        node = AVLNode[K,V](None, k, v)
        if self.root is None:
            # The root's parent is None.
            self.root = node
        else:
            self.root.insert(node)
        self.rebalance(node)

    def delete(self, k):
        node = self.find(k)
        if node is None:
            return
        deleted = None
        if node is self.root:
            pseudoroot = AVLNode[K,V](None, 0, 0)
            pseudoroot.left = self.root
            self.root.parent = pseudoroot
            deleted = self.root.delete()
            self.root = pseudoroot.left
            if self.root is not None:
                self.root.parent = None
        else:
            deleted = node.delete()
        self.rebalance(deleted.parent)

    def __setitem__(self: AVL[K,V], k: K, v: V):
        self.insert(k, v)

    def __getitem__(self: AVL[K,V], k: K):
        nd = self.find(k)
        if not nd:
            print 'whoops', k, 'not found'
            sys.exit(1)
        return nd.value

    def __delitem__(self: AVL[K,V], k: K):
        self.delete(k)

    def __contains__(self: AVL[K,V], k: K):
        return self.find(k) is not None

    def __iter__(self: AVL[K,V]):
        if self.root:
            for i in self.root:
                yield i.key, i.value

d1 = AVL[int,int]()
for a in range(5):
    d1[a] = a*a

# EXPECT: 0
# EXPECT: 1
# EXPECT: 4
# EXPECT: 9
# EXPECT: 16
for a in range(5):
    print d1[a]

print 2 in d1  # EXPECT: True
del d1[2]
print 2 in d1  # EXPECT: False
d1[2] = 44
print 2 in d1  # EXPECT: True
print d1[2]    # EXPECT: 44

del d1[3]
del d1[4]

# EXPECT: 0 0
# EXPECT: 1 1
# EXPECT: 2 44
for t in d1:
    print t[0], t[1]
