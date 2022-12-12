#include <chrono>
#include <fstream>
#include <iostream>
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
  
  if (file)
  {
    std::string str;
    
    file.seekg(0, std::ios::end);
    str.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());
    
    unordered_map<string_view, uint32_t> map;
    string_view line;
    auto c = str.data();
    auto start = c;
    while (*c != '\0') {
      if (*c == '\n') {
        line = { start, static_cast<size_t>(c - start) };
        ++map[line];
        
        ++c;
        start = c;
      }
      else {
        ++c;
      }
    }
    
    // final word
    line = { start, static_cast<size_t>(c - start) };
    ++map[line];

    cout << map.size() << '\n';
    cout << (duration_cast<milliseconds>(clock::now() - t).count() / 1e3) << endl;
  }
  else {
    cerr << "Could not open file: " << argv[1] << endl;
    return -1;
  }
}
