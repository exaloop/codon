#include "codon/runtime/lib.h"
#include <cstring>
#include <string>
#include <unordered_map>
#include <re2/re2.h>

std::unordered_map<std::string, std::unique_ptr<re2::RE2>> cache;

static inline re2::RE2& get(std::string &p) {
  auto it = cache.find(p);
  if (it == cache.end()) {
    auto result = cache.emplace(p, std::make_unique<re2::RE2>(p));
    return *result.first->second;
  } else {
    return *it->second;
  }
}

SEQ_FUNC bool seq_re_fullmatch(seq_str_t p, seq_str_t s) {
  std::string pattern(p.str, p.len);
  std::string string(s.str, s.len);
  return re2::RE2::FullMatch(string, get(pattern));
}

SEQ_FUNC seq_str_t *seq_re_findall(seq_str_t p, seq_str_t s, seq_int_t *count, seq_int_t *capacity) {
#define INIT_BUFFER_SIZE 3
  std::string pattern(p.str, p.len);
  std::string string(s.str, s.len);
  pattern = "(" + pattern + ")";
  auto &regex = get(pattern);
  re2::StringPiece input(string);
  std::string match;

  seq_int_t n = 0;
  seq_int_t m = INIT_BUFFER_SIZE;
  auto *matches = (seq_str_t *)seq_alloc_atomic(m * sizeof(seq_str_t));

  while (RE2::FindAndConsume(&input, regex, &match)) {
    if (n == m) {
      m = (1 + 3*m) / 2;
      matches = (seq_str_t *)seq_realloc(matches, m * sizeof(seq_str_t));
    }

    seq_int_t match_size = match.size();
    auto *s = (char *)seq_alloc_atomic(match_size);
    std::memcpy(s, match.data(), match_size);
    matches[n++] = {match_size, s};
  }

  *count = n;
  *capacity = m;
  return matches;
#undef INIT_BUFFER_SIZE
}
