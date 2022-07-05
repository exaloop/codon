#include "codon/runtime/lib.h"
#include <cstring>
#include <re2/re2.h>
#include <string>
#include <unordered_map>
#include <vector>

/*
 * Internal helpers & utilities
 */

using Regex = re2::RE2;
using re2::StringPiece;

struct Span {
  seq_int_t start;
  seq_str_t stop;
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

static std::unordered_map<std::string, std::unique_ptr<Regex>> cache;

static inline Regex &get(std::string &p) {
  auto it = cache.find(p);
  if (it == cache.end()) {
    auto result = cache.emplace(p, std::make_unique<Regex>(p));
    return *result.first->second;
  } else {
    return *it->second;
  }
}

static inline seq_str_t convert(const std::string &p) {
  seq_int_t n = p.size();
  auto *s = (char *)seq_alloc_atomic(n);
  std::memcpy(s, p.data(), n);
  return {n, s};
}

static inline StringPiece str2sp(const seq_str_t &s) {
  return StringPiece(s.str, s.len);
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

SEQ_FUNC Span seq_re_match_one(Regex *re, seq_int_t anchor, seq_str_t s, seq_int_t pos,
                               seq_int_t endpos) {
  StringPiece m;
  if (!re->Match(str2sp(s), pos, endpos, static_cast<Regex::Anchor>(anchor), &m, 1))
    return {-1, -1};

  return {static_cast<seq_int_t>(m.data() - s.str),
          static_cast<seq_int_t>(m.data() - s.str + m.size())};
}

/*
 * General functions
 */

SEQ_FUNC seq_str_t seq_re_escape(seq_str_t p) {
  return convert(Regex::QuoteMeta(str2sp(p)));
}

SEQ_FUNC void *seq_re_compile(seq_str_t p) {
  return new (seq_alloc_atomic(sizeof(Regex))) Regex(str2sp(p));
}

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

SEQ_FUNC seq_int_t seq_re_pattern_groupindex(Regex *pattern, seq_str_t **names,
                                             seq_int_t **indices) {
  const int num_groups = pattern->NumberOfCapturingGroups();
  if (num_groups == 0)
    return 0;

  *names = (seq_str_t *)seq_alloc_atomic(num_groups * sizeof(seq_str_t));
  *indices = (seq_int_t *)seq_alloc_atomic(num_groups * sizeof(seq_int_t));
  auto mapping = pattern->NamedCapturingGroups();
  unsigned i = 0;

  for (const auto &it : mapping) {
    (*names)[i] = convert(it.first);
    (*indices)[i] = it.second;
    ++i;
  }

  return num_groups;
}

SEQ_FUNC bool seq_re_pattern_search(Regex *pattern, seq_str_t string, seq_int_t pos,
                                    seq_int_t endpos) {
  return Regex::FullMatch(str2sp(string), *pattern);
}

SEQ_FUNC bool seq_re_pattern_fullmatch(Regex *re, seq_str_t s) {
  return Regex::FullMatch(str2sp(s), *re);
}

/*
 * Module-level functions
 */

SEQ_FUNC bool seq_re_fullmatch(seq_str_t p, seq_str_t s) {
  std::string pattern(p.str, p.len);
  return seq_re_pattern_fullmatch(&get(pattern), s);
}

SEQ_FUNC seq_str_t *seq_re_findall(seq_str_t p, seq_str_t s, seq_int_t *count,
                                   seq_int_t *capacity) {
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
      m = (1 + 3 * m) / 2;
      matches = (seq_str_t *)seq_realloc(matches, m * sizeof(seq_str_t));
    }
    matches[n++] = convert(match);
  }

  *count = n;
  *capacity = m;
  return matches;
#undef INIT_BUFFER_SIZE
}
