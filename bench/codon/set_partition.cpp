#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <vector>

template <class T> using vec = std::vector<T>;

inline vec<int> range(int start, int stop) {
  vec<int> v(stop - start);
  uint j = 0;
  for (int i = start; i < stop; i++)
    v[j++] = i;
  return v;
}

inline bool conforms(const vec<vec<int>> &candidate, int minsize, int forgive) {
  int deficit = 0;
  for (const auto &p : candidate) {
    int need = minsize - static_cast<int>(p.size());
    if (need > 0)
      deficit += need;
  }
  return deficit <= forgive;
}

inline void partition_filtered(const vec<int> &collection,
                               std::function<void(const vec<vec<int>> &)> callback,
                               int minsize = 1, int forgive = 0) {
  if (collection.size() == 1) {
    callback({collection});
    return;
  }

  auto first = collection[0];
  auto loop = [&](const vec<vec<int>> &smaller) {
    int n = 0;

    vec<vec<int>> candidate;
    candidate.reserve(smaller.size() + 1);
    vec<int> rep;

    for (const auto &subset : smaller) {
      candidate.resize(n);
      for (int i = 0; i < n; i++)
        candidate[i] = smaller[i];

      rep.clear();
      rep.reserve(subset.size() + 1);
      rep.push_back(first);
      rep.insert(rep.end(), subset.begin(), subset.end());
      candidate.push_back({rep});

      for (int i = n + 1; i < smaller.size(); i++)
        candidate.push_back(smaller[i]);

      if (conforms(candidate, minsize, forgive))
        callback(candidate);
      ++n;
    }

    candidate.clear();
    candidate.push_back({first});
    candidate.insert(candidate.end(), smaller.begin(), smaller.end());

    if (conforms(candidate, minsize, forgive))
      callback(candidate);
  };

  vec<int> new_collection(collection.begin() + 1, collection.end());
  partition_filtered(new_collection, loop, minsize, forgive + 1);
}

int main(int argc, char *argv[]) {
  using clock = std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  auto t = clock::now();
  int n = 1;
  int x = 0;

  auto callback = [&](const vec<vec<int>> &p) {
    auto copy = p;
    std::sort(copy.begin(), copy.end());
    x += copy[copy.size() / 3][0];
  };

  auto something = range(1, std::atoi(argv[1]));
  partition_filtered(something, callback, 2);
  std::cout << x << std::endl;
  std::cout << (duration_cast<milliseconds>(clock::now() - t).count() / 1e3)
            << std::endl;
}
