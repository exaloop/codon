// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/runtime/lib.h"
#include <cstring>
#include <re2/re2.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using Regex = re2::RE2;
using re2::StringPiece;

/*
 * Flags -- (!) must match Codon's
 */

#define ASCII (1 << 0)
#define DEBUG (1 << 1)
#define IGNORECASE (1 << 2)
#define LOCALE (1 << 3)
#define MULTILINE (1 << 4)
#define DOTALL (1 << 5)
#define VERBOSE (1 << 6)

static inline Regex::Options flags2opt(seq_int_t flags, bool is_bytes) {
  Regex::Options opt;
  opt.set_log_errors(false);
  opt.set_encoding(is_bytes ? Regex::Options::Encoding::EncodingLatin1
                            : Regex::Options::Encoding::EncodingUTF8);

  if (flags & ASCII) {
    // nothing
  }

  if (flags & DEBUG) {
    // nothing
  }

  if (flags & IGNORECASE) {
    opt.set_case_sensitive(false);
  }

  if (flags & LOCALE) {
    // nothing
  }

  if (flags & MULTILINE) {
    opt.set_one_line(false);
  }

  if (flags & DOTALL) {
    opt.set_dot_nl(true);
  }

  if (flags & VERBOSE) {
    // nothing
  }

  return opt;
}

/*
 * Internal helpers & utilities
 */

struct Span {
  seq_int_t start;
  seq_int_t end;
};

template <typename KV> struct GCMapAllocator : public std::allocator<KV> {
  GCMapAllocator() = default;
  GCMapAllocator(GCMapAllocator<KV> const &) = default;

  template <typename KV1> GCMapAllocator(const GCMapAllocator<KV1> &) noexcept {}

  KV *allocate(std::size_t n) { return (KV *)seq_alloc_uncollectable(n * sizeof(KV)); }

  void deallocate(KV *p, std::size_t n) { seq_free(p); }

  template <typename U> struct rebind {
    using other = GCMapAllocator<U>;
  };
};

static inline seq_bytes_t convert(const std::string &p) {
  seq_int_t n = p.size();
  auto *s = (char *)seq_alloc_atomic(n);
  std::memcpy(s, p.data(), n);
  return {s, n};
}

static inline StringPiece bytes2sp(const seq_bytes_t &s) {
  return StringPiece(s.ptr, s.len);
}

using Key = std::pair<std::string, seq_int_t>;

struct KeyHash {
  std::size_t operator()(const Key &k) const {
    return std::hash<std::string>()(k.first) ^ k.second;
  }
};

static thread_local std::unordered_map<const Key, Regex, KeyHash, std::equal_to<Key>,
                                       GCMapAllocator<std::pair<const Key, Regex>>>
    cache;

static inline Regex *get(const seq_bytes_t &p, seq_int_t flags, bool is_bytes) {
  auto key_flags = flags | (is_bytes ? (seq_int_t(1) << 32) : 0);
  auto key = std::make_pair(std::string(p.ptr, p.len), key_flags);
  auto it = cache.find(key);
  if (it == cache.end()) {
    auto result =
        cache.emplace(std::piecewise_construct, std::forward_as_tuple(key),
                      std::forward_as_tuple(key.first, flags2opt(flags, is_bytes)));
    return &result.first->second;
  } else {
    return &it->second;
  }
}

/*
 * Matching
 */

SEQ_FUNC Span *seq_re_match(Regex *re, seq_int_t anchor, seq_bytes_t s, seq_int_t pos,
                            seq_int_t endpos) {
  const int num_groups = re->NumberOfCapturingGroups() + 1; // need $0
  std::vector<StringPiece> groups;
  groups.resize(num_groups);

  if (!re->Match(bytes2sp(s), pos, endpos, static_cast<Regex::Anchor>(anchor),
                 groups.data(), groups.size())) {
    // Ensure that groups are null before converting to spans!
    for (auto &it : groups) {
      it = StringPiece();
    }
  }

  auto *spans = (Span *)seq_alloc_atomic(num_groups * sizeof(Span));
  unsigned i = 0;
  for (const auto &it : groups) {
    if (it.data() == nullptr) {
      spans[i++] = {-1, -1};
    } else {
      spans[i++] = {static_cast<seq_int_t>(it.data() - s.ptr),
                    static_cast<seq_int_t>(it.data() - s.ptr + it.size())};
    }
  }

  return spans;
}

SEQ_FUNC Span seq_re_match_one(Regex *re, seq_int_t anchor, seq_bytes_t s,
                               seq_int_t pos, seq_int_t endpos) {
  StringPiece m;
  if (!re->Match(bytes2sp(s), pos, endpos, static_cast<Regex::Anchor>(anchor), &m, 1))
    return {-1, -1};
  else
    return {static_cast<seq_int_t>(m.data() - s.ptr),
            static_cast<seq_int_t>(m.data() - s.ptr + m.size())};
}

/*
 * General functions
 */

SEQ_FUNC seq_bytes_t seq_re_escape(seq_bytes_t p) {
  return convert(Regex::QuoteMeta(bytes2sp(p)));
}

SEQ_FUNC Regex *seq_re_compile(seq_bytes_t p, seq_int_t flags, bool is_bytes) {
  return get(p, flags, is_bytes);
}

SEQ_FUNC void seq_re_purge() { cache.clear(); }

/*
 * Pattern methods
 */

SEQ_FUNC seq_int_t seq_re_pattern_groups(Regex *pattern) {
  return pattern->NumberOfCapturingGroups();
}

SEQ_FUNC seq_int_t seq_re_group_name_to_index(Regex *pattern, seq_bytes_t name) {
  const auto &mapping = pattern->NamedCapturingGroups();
  auto it = mapping.find(std::string(name.ptr, name.len));
  return (it != mapping.end()) ? it->second : -1;
}

SEQ_FUNC seq_bytes_t seq_re_group_index_to_name(Regex *pattern, seq_int_t index) {
  const auto &mapping = pattern->CapturingGroupNames();
  auto it = mapping.find(index);
  seq_bytes_t empty = {nullptr, 0};
  return (it != mapping.end()) ? convert(it->second) : empty;
}

SEQ_FUNC bool seq_re_check_rewrite_string(Regex *pattern, seq_bytes_t rewrite,
                                          seq_bytes_t *error) {
  std::string e;
  bool ans = pattern->CheckRewriteString(bytes2sp(rewrite), &e);
  if (!ans)
    *error = convert(e);
  return ans;
}

SEQ_FUNC seq_bytes_t seq_re_pattern_error(Regex *pattern) {
  if (pattern->ok())
    return {nullptr, 0};
  return convert(pattern->error());
}
