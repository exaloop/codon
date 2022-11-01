#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace std;

int main(int argc, char *argv[]) {
  using clock = chrono::high_resolution_clock;
  using chrono::duration_cast;
  using chrono::milliseconds;
  auto t = clock::now();

  cin.tie(nullptr);
  cout.sync_with_stdio(false);

  if (argc != 2) {
    cerr << "Expected one argument." << endl;
    return -1;
  }

  ifstream file(argv[1]);
  if (!file.is_open()) {
    cerr << "Could not open file: " << argv[1] << endl;
    return -1;
  }

  unordered_map<string, int> map;
  for (string line; getline(file, line);) {
    istringstream sin(line);
    for (string word; sin >> word;)
      map[word] += 1;
  }

  cout << map.size() << endl;
  cout << (duration_cast<milliseconds>(clock::now() - t).count() / 1e3) << endl;
}
