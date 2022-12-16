#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

struct Node {
  Node* left{};
  Node* right{};
};

inline Node* make_tree(int d, std::vector<Node>& pool) {
  if (d > 0) {
    return &pool.emplace_back(Node{make_tree(d - 1, pool), make_tree(d - 1, pool)});
  }
  else {
    return &pool.emplace_back(Node{});
  }
}

inline int check_tree(const Node* node) {
  if (!node->left)
    return 1;
  else
    return 1 + check_tree(node->left) + check_tree(node->right);
}

inline int make_check(const std::pair<int, int> &itde, std::vector<Node>& pool) {
  int i = itde.first, d = itde.second;
  pool.clear();
  auto tree = make_tree(d, pool);
  return check_tree(tree);
}

struct ArgChunks {
  int i, k, d, chunksize;
  std::vector<std::pair<int, int>> chunk;

  ArgChunks(int i, int d, int chunksize = 5000)
      : i(i), k(1), d(d), chunksize(chunksize), chunk() {
    assert(chunksize % 2 == 0);
  }

  bool next() {
    chunk.clear();
    while (k <= i) {
      chunk.emplace_back(k++, d);
      if (chunk.size() == chunksize)
        return true;
    }
    return !chunk.empty();
  }
};

int main(int argc, char *argv[]) {
  using clock = std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  auto t = clock::now();
  int min_depth = 4;
  int n = std::stoi(argv[1]);
  int max_depth = std::max(min_depth + 2, n);
  int stretch_depth = max_depth + 1;
  
  std::vector<Node> pool{};

  std::cout << "stretch tree of depth " << stretch_depth
            << "\t check: " << make_check({0, stretch_depth}, pool) << '\n';
  
  auto long_lived_tree = make_tree(max_depth, pool);
  int mmd = max_depth + min_depth;
  for (int d = min_depth; d < stretch_depth; d += 2) {
    int i = (1 << (mmd - d));
    int cs = 0;
    ArgChunks iter(i, d);
    while (iter.next()) {
      for (auto &argchunk : iter.chunk) {
        cs += make_check(argchunk, pool);
      }
    }
    std::cout << i << "\t trees of depth " << d << "\t check: " << cs << '\n';
  }
  std::cout << "long lived tree of depth " << max_depth
            << "\t check: " << check_tree(long_lived_tree) << '\n';
  std::cout << (duration_cast<milliseconds>(clock::now() - t).count() / 1e3)
            << std::endl;
}
