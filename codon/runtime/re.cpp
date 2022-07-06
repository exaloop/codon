#include "codon/runtime/lib.h"
#include <cstring>
#include <re2/re2.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

/*
 * Internal helpers & utilities
 */

using Regex = re2::RE2;
using re2::StringPiece;

struct Span {
  seq_int_t start;
  seq_int_t end;
};

// Caution: must match Codon's implementations

struct Pattern {
  seq_str_t pattern;
  seq_int_t flags;
  Regex *re;
};

struct Match {
  Span *spans;
  seq_int_t pos;
  seq_int_t endpos;
  Pattern re;
  seq_str_t string;
};

template <class Key, class Value>
struct GCMapAllocator : public std::allocator<std::pair<const Key, Value>> {
  using value_type = std::pair<const Key, Value>;

  value_type *allocate(std::size_t n) {
    return (value_type *)seq_alloc(n * sizeof(value_type));
  }

  void deallocate(value_type *p, std::size_t n) { seq_gc_free(p); }
};

static inline seq_str_t convert(const std::string &p) {
  seq_int_t n = p.size();
  auto *s = (char *)seq_alloc(n);
  std::memcpy(s, p.data(), n);
  return {n, s};
}

static inline StringPiece str2sp(const seq_str_t &s) {
  return StringPiece(s.str, s.len);
}

using Key = std::pair<seq_str_t, seq_int_t>;

struct KeyEqual {
  bool operator()(const Key &a, const Key &b) const {
    return a.second == b.second && str2sp(a.first) == str2sp(b.first);
  }
};

struct KeyHash {
  std::size_t operator()(const Key &k) const {
    using sv = std::string_view;
    return std::hash<sv>()(sv(k.first.str, k.first.len)) ^ k.second;
  }
};

static thread_local std::unordered_map<Key, Regex, KeyHash, KeyEqual,
                                       GCMapAllocator<Key, Regex>>
    cache;

static inline Regex *get(const seq_str_t &p, seq_int_t flags) {
  auto key = std::make_pair(p, flags);
  auto it = cache.find(key);
  if (it == cache.end()) {
    auto result = cache.emplace(key, str2sp(p));
    return &result.first->second;
  } else {
    return &it->second;
  }
}

/*
 * Matching
 */

SEQ_FUNC Span *seq_re_match(Regex *re, seq_int_t anchor, seq_str_t s, seq_int_t pos,
                            seq_int_t endpos) {
  const int num_groups = re->NumberOfCapturingGroups() + 1; // need $0
  std::vector<StringPiece> groups;
  groups.resize(num_groups);

  if (!re->Match(str2sp(s), pos, endpos, static_cast<Regex::Anchor>(anchor),
                 groups.data(), groups.size())) {
    // Ensure that groups are null before converting to spans!
    for (auto &it : groups) {
      it = StringPiece();
    }
  }

  auto *spans = (Span *)seq_alloc_atomic(num_groups * sizeof(Span));
  unsigned i = 0;
  for (const auto &it : groups) {
    if (it.data() == NULL) {
      spans[i++] = {-1, -1};
    } else {
      spans[i++] = {static_cast<seq_int_t>(it.data() - s.str),
                    static_cast<seq_int_t>(it.data() - s.str + it.size())};
    }
  }

  return spans;
}

SEQ_FUNC void seq_re_match_one(Regex *re, seq_int_t anchor, seq_str_t s, seq_int_t pos,
                               seq_int_t endpos, Span *span) {
  StringPiece m;
  if (!re->Match(str2sp(s), pos, endpos, static_cast<Regex::Anchor>(anchor), &m, 1))
    *span = {-1, -1};
  else
    *span = {static_cast<seq_int_t>(m.data() - s.str),
             static_cast<seq_int_t>(m.data() - s.str + m.size())};
}

/*
 * General functions
 */

SEQ_FUNC seq_str_t seq_re_escape(seq_str_t p) {
  return convert(Regex::QuoteMeta(str2sp(p)));
}

SEQ_FUNC Regex *seq_re_compile(seq_str_t p, seq_int_t flags) { return get(p, flags); }

SEQ_FUNC void seq_re_purge() { cache.clear(); }

/*
 * Match methods
 */

SEQ_FUNC seq_str_t seq_re_match_expand(Match *match, seq_str_t templ) {
  // TODO
  return {0, nullptr};
}

/*
 * Pattern methods
 */

SEQ_FUNC seq_int_t seq_re_pattern_groups(Regex *pattern) {
  return pattern->NumberOfCapturingGroups();
}

SEQ_FUNC seq_int_t seq_re_group_name_to_index(Regex *pattern, seq_str_t name) {
  const auto &mapping = pattern->NamedCapturingGroups();
  auto it = mapping.find(std::string(name.str, name.len));
  return (it != mapping.end()) ? it->second : -1;
}

SEQ_FUNC seq_int_t seq_re_pattern_groupindex(Regex *pattern, seq_str_t **names,
                                             seq_int_t **indices) {
  const int num_groups = pattern->NumberOfCapturingGroups();
  if (num_groups == 0)
    return 0;

  *names = (seq_str_t *)seq_alloc_atomic(num_groups * sizeof(seq_str_t));
  *indices = (seq_int_t *)seq_alloc_atomic(num_groups * sizeof(seq_int_t));
  const auto &mapping = pattern->NamedCapturingGroups();
  unsigned i = 0;

  for (const auto &it : mapping) {
    (*names)[i] = convert(it.first);
    (*indices)[i] = it.second;
    ++i;
  }

  return num_groups;
}

SEQ_FUNC bool seq_re_check_rewrite_string(Regex *pattern, seq_str_t rewrite,
                                          seq_str_t *error) {
  std::string e;
  bool ans = pattern->CheckRewriteString(str2sp(rewrite), &e);
  if (!ans)
    *error = convert(e);
  return ans;
}
