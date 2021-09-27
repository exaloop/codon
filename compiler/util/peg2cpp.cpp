
#include "compiler/util/peglib.h"
#include <algorithm>
#include <any>
#include <cassert>
#include <cctype>
#include <cstring>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define FMT_HEADER_ONLY
#include "compiler/util/fmt/format.h"

using namespace std;

string escape(const string &str) {
  string r;
  for (unsigned char c : str) {
    switch (c) {
    case '\n':
      r += "\\\\n";
      break;
    case '\r':
      r += "\\\\r";
      break;
    case '\t':
      r += "\\\\t";
      break;
    case '\\':
      r += "\\\\";
      break;
    case '"':
      r += "\\\"";
      break;
    default:
      if (c < 32 || c >= 127)
        r += fmt::format("\\\\x{:x}", c);
      else
        r += c;
    }
  }
  return r;
}
template <typename T>
string join(const T &items, const string &delim = " ", int start = 0, int end = -1) {
  string s;
  if (end == -1)
    end = items.size();
  for (int i = start; i < end; i++)
    s += (i > start ? delim : "") + items[i];
  return s;
}

const string NO_PACKRAT = ":NO_PACKRAT";

namespace peg {
using Rules = std::unordered_map<std::string, std::shared_ptr<Ope>>;

struct SetUpPackrat : public Ope::Visitor {
  bool packrat;
  unordered_set<string> *seen;

  static bool check(unordered_set<string> *seen, const shared_ptr<Ope> &op) {
    SetUpPackrat v;
    v.seen = seen;
    v.packrat = true;
    op->accept(v);
    return v.packrat;
  };

  void visit(Sequence &ope) override {
    for (auto op : ope.opes_)
      packrat &= check(seen, op);
  }
  void visit(PrioritizedChoice &ope) override {
    for (auto op : ope.opes_)
      packrat &= check(seen, op);
  }
  void visit(Repetition &ope) override { packrat &= check(seen, ope.ope_); }
  void visit(AndPredicate &ope) override { packrat &= check(seen, ope.ope_); }
  void visit(NotPredicate &ope) override { packrat &= check(seen, ope.ope_); }
  void visit(CaptureScope &ope) override { packrat &= check(seen, ope.ope_); }
  void visit(Capture &ope) override { packrat &= check(seen, ope.ope_); }
  void visit(TokenBoundary &ope) override { packrat &= check(seen, ope.ope_); }
  void visit(Ignore &ope) override { packrat &= check(seen, ope.ope_); }
  void visit(WeakHolder &ope) override { packrat &= check(seen, ope.weak_.lock()); }
  void visit(Holder &ope) override { packrat &= check(seen, ope.ope_); }
  void visit(Reference &ope) override {
    if (seen->find(ope.name_) != seen->end()) {
      if (ope.rule_)
        packrat &= ope.rule_->enable_memoize;
      return;
    }
    seen->insert(ope.name_);
    for (auto op : ope.args_)
      packrat &= check(seen, op);
    if (ope.rule_) {
      if (auto op = ope.get_core_operator())
        packrat &= check(seen, op);
      packrat &= ope.rule_->enable_memoize;
      if (!packrat)
        ope.rule_->enable_memoize = false;
    }
  }
  void visit(Whitespace &ope) override { packrat &= check(seen, ope.ope_); }
  void visit(PrecedenceClimbing &ope) override { packrat &= check(seen, ope.atom_); }
  void visit(Recovery &ope) override { packrat &= check(seen, ope.ope_); }
};

class ParserGenerator {
public:
  static ParserGenerator &get_instance() {
    static ParserGenerator instance;
    return instance;
  }

private:
  ParserGenerator() {
    make_grammar();
    setup_actions();
  }

  struct Instruction {
    std::string type;
    std::any data;
  };

  struct Data {
    std::shared_ptr<Grammar> grammar;
    std::string start;
    const char *start_pos = nullptr;
    std::vector<std::pair<std::string, const char *>> duplicates;
    std::map<std::string, Instruction> instructions;
    std::set<std::string_view> captures;
    bool enablePackratParsing = true;
    std::string preamble;

    Data() : grammar(std::make_shared<Grammar>()) {}
  };

  void make_grammar() {
    // Setup PEG syntax parser
    g["Grammar"] <= seq(g["Spacing"], oom(g["Definition"]), g["EndOfFile"]);
    g["Definition"] <=
        cho(seq(g["Ignore"], g["IdentCont"], g["Parameters"], g["LEFTARROW"],
                g["TopExpression"], opt(g["Instruction"])),
            seq(g["Ignore"], g["Identifier"], g["LEFTARROW"], g["TopExpression"],
                opt(g["Instruction"])),
            seq(g["Ignore"], lit("%preamble"), g["Spacing"], g["CppInstr"]));
    g["TopExpression"] <=
        seq(opt(g["SLASH"]), g["TopChoice"], zom(seq(g["SLASH"], g["TopChoice"])));
    g["TopChoice"] <= seq(g["Sequence"], opt(g["CppInstr"]));
    g["Expression"] <= seq(g["Sequence"], zom(seq(g["SLASH"], g["Sequence"])));
    g["Sequence"] <= zom(cho(g["CUT"], g["Prefix"]));
    g["Prefix"] <= seq(opt(cho(g["AND"], g["NOT"])), g["SuffixWithLabel"]);
    g["SuffixWithLabel"] <= seq(g["Suffix"], opt(seq(g["LABEL"], g["Identifier"])));
    g["Suffix"] <= seq(g["Primary"], opt(g["Loop"]));
    g["Loop"] <= cho(g["QUESTION"], g["STAR"], g["PLUS"], g["Repetition"]);
    g["Primary"] <=
        cho(seq(g["Ignore"], g["IdentCont"], g["Arguments"], npd(g["LEFTARROW"])),
            seq(g["Ignore"], g["Identifier"],
                npd(seq(opt(g["Parameters"]), g["LEFTARROW"]))),
            seq(g["OPEN"], g["Expression"], g["CLOSE"]),
            seq(g["BeginTok"], g["Expression"], g["EndTok"]),
            seq(g["BeginCapScope"], g["Expression"], g["EndCapScope"]),
            seq(g["BeginCap"], g["Expression"], g["EndCap"]), g["BackRef"],
            g["LiteralI"], g["Dictionary"], g["Literal"], g["NegatedClass"], g["Class"],
            g["DOT"]);

    g["Identifier"] <=
        seq(g["IdentCont"], opt(tok(lit(string(NO_PACKRAT)))), g["Spacing"]);
    g["IdentCont"] <= seq(g["IdentStart"], zom(g["IdentRest"]));

    const static std::vector<std::pair<char32_t, char32_t>> range = {{0x0080, 0xFFFF}};
    g["IdentStart"] <=
        seq(npd(lit(u8(u8"↑"))), npd(lit(u8(u8"⇑"))), cho(cls("a-zA-Z_%"), cls(range)));

    g["IdentRest"] <= cho(g["IdentStart"], cls("0-9"));

    g["Dictionary"] <= seq(g["LiteralD"], oom(seq(g["PIPE"], g["LiteralD"])));

    auto lit_ope = cho(
        seq(cls("'"), tok(zom(seq(npd(cls("'")), g["Char"]))), cls("'"), g["Spacing"]),
        seq(cls("\""), tok(zom(seq(npd(cls("\"")), g["Char"]))), cls("\""),
            g["Spacing"]));
    g["Literal"] <= lit_ope;
    g["LiteralD"] <= lit_ope;

    g["LiteralI"] <= cho(seq(cls("'"), tok(zom(seq(npd(cls("'")), g["Char"]))),
                             lit("'i"), g["Spacing"]),
                         seq(cls("\""), tok(zom(seq(npd(cls("\"")), g["Char"]))),
                             lit("\"i"), g["Spacing"]));

    // NOTE: The original Brian Ford's paper uses 'zom' instead of 'oom'.
    g["Class"] <= seq(chr('['), npd(chr('^')), tok(oom(seq(npd(chr(']')), g["Range"]))),
                      chr(']'), g["Spacing"]);
    g["NegatedClass"] <= seq(lit("[^"), tok(oom(seq(npd(chr(']')), g["Range"]))),
                             chr(']'), g["Spacing"]);

    g["Range"] <= cho(seq(g["Char"], chr('-'), g["Char"]), g["Char"]);
    g["Char"] <=
        cho(seq(chr('\\'), cls("nrt'\"[]\\^")),
            seq(chr('\\'), cls("0-3"), cls("0-7"), cls("0-7")),
            seq(chr('\\'), cls("0-7"), opt(cls("0-7"))),
            seq(lit("\\x"), cls("0-9a-fA-F"), opt(cls("0-9a-fA-F"))),
            seq(lit("\\u"), cho(seq(cho(seq(chr('0'), cls("0-9a-fA-F")), lit("10")),
                                    rep(cls("0-9a-fA-F"), 4, 4)),
                                rep(cls("0-9a-fA-F"), 4, 5))),
            seq(npd(chr('\\')), dot()));

    g["Repetition"] <= seq(g["BeginBlacket"], g["RepetitionRange"], g["EndBlacket"]);
    g["RepetitionRange"] <= cho(seq(g["Number"], g["COMMA"], g["Number"]),
                                seq(g["Number"], g["COMMA"]), g["Number"],
                                seq(g["COMMA"], g["Number"]));
    g["Number"] <= seq(oom(cls("0-9")), g["Spacing"]);

    g["LEFTARROW"] <= seq(cho(lit("<-"), lit(u8(u8"←"))), g["Spacing"]);
    ~g["SLASH"] <= seq(chr('/'), g["Spacing"]);
    ~g["PIPE"] <= seq(chr('|'), g["Spacing"]);
    g["AND"] <= seq(chr('&'), g["Spacing"]);
    g["NOT"] <= seq(chr('!'), g["Spacing"]);
    g["QUESTION"] <= seq(chr('?'), g["Spacing"]);
    g["STAR"] <= seq(chr('*'), g["Spacing"]);
    g["PLUS"] <= seq(chr('+'), g["Spacing"]);
    ~g["OPEN"] <= seq(chr('('), g["Spacing"]);
    ~g["CLOSE"] <= seq(chr(')'), g["Spacing"]);
    g["DOT"] <= seq(chr('.'), g["Spacing"]);

    g["CUT"] <= seq(chr('^'), g["Spacing"]);    // Change from ↑ to ^
    ~g["LABEL"] <= seq(chr('@'), g["Spacing"]); // Change from ⇑ to @

    ~g["Spacing"] <= zom(cho(g["Space"], g["Comment"]));
    g["Comment"] <= seq(chr('#'), zom(seq(npd(g["EndOfLine"]), dot())), g["EndOfLine"]);
    g["Space"] <= cho(chr(' '), chr('\t'), g["EndOfLine"]);
    g["EndOfLine"] <= cho(lit("\r\n"), chr('\n'), chr('\r'));
    g["EndOfFile"] <= npd(dot());

    ~g["BeginTok"] <= seq(chr('<'), g["Spacing"]);
    ~g["EndTok"] <= seq(chr('>'), g["Spacing"]);

    ~g["BeginCapScope"] <= seq(chr('$'), chr('('), g["Spacing"]);
    ~g["EndCapScope"] <= seq(chr(')'), g["Spacing"]);

    g["BeginCap"] <= seq(chr('$'), tok(g["IdentCont"]), chr('<'), g["Spacing"]);
    ~g["EndCap"] <= seq(chr('>'), g["Spacing"]);

    g["BackRef"] <= seq(chr('$'), tok(g["IdentCont"]), g["Spacing"]);

    g["IGNORE"] <= chr('~');

    g["Ignore"] <= opt(g["IGNORE"]);
    g["Parameters"] <= seq(g["OPEN"], g["Identifier"],
                           zom(seq(g["COMMA"], g["Identifier"])), g["CLOSE"]);
    g["Arguments"] <= seq(g["OPEN"], g["Expression"],
                          zom(seq(g["COMMA"], g["Expression"])), g["CLOSE"]);
    ~g["COMMA"] <= seq(chr(','), g["Spacing"]);

    // Instruction grammars
    g["Instruction"] <= seq(g["BeginBlacket"],
                            cho(cho(g["PrecedenceClimbing"]), cho(g["ErrorMessage"]),
                                cho(g["NoAstOpt"])),
                            g["EndBlacket"]);

    ~g["SpacesZom"] <= zom(g["Space"]);
    ~g["SpacesOom"] <= oom(g["Space"]);
    ~g["BeginBlacket"] <= seq(chr('{'), g["Spacing"]);
    ~g["EndBlacket"] <= seq(chr('}'), g["Spacing"]);

    // PrecedenceClimbing instruction
    g["PrecedenceClimbing"] <=
        seq(lit("precedence"), g["SpacesOom"], g["PrecedenceInfo"],
            zom(seq(g["SpacesOom"], g["PrecedenceInfo"])), g["SpacesZom"]);
    g["PrecedenceInfo"] <=
        seq(g["PrecedenceAssoc"], oom(seq(ign(g["SpacesOom"]), g["PrecedenceOpe"])));
    g["PrecedenceOpe"] <=
        cho(seq(cls("'"), tok(zom(seq(npd(cho(g["Space"], cls("'"))), g["Char"]))),
                cls("'")),
            seq(cls("\""), tok(zom(seq(npd(cho(g["Space"], cls("\""))), g["Char"]))),
                cls("\"")),
            tok(oom(seq(npd(cho(g["PrecedenceAssoc"], g["Space"], chr('}'))), dot()))));
    g["PrecedenceAssoc"] <= cls("LR");

    // Error message instruction (change "message" to "!")
    g["ErrorMessage"] <= seq(lit("!"), g["SpacesOom"], g["LiteralD"], g["SpacesZom"]);

    // No Ast node optimazation instruction
    g["NoAstOpt"] <= seq(lit("no_ast_opt"), g["SpacesZom"]);

    g["CppInstr"] <= seq(g["CppCode"], g["Spacing"]);
    g["CppCode"] <= seq(chr('{'), zom(g["CppChar"]), chr('}'));
    g["CppChar"] <= cho(g["CppCode"], seq(npd(chr('{')), npd(chr('}')), dot()));

    // Set definition names
    for (auto &x : g) {
      x.second.name = x.first;
    }
  }

  void setup_actions() {
    g["Definition"] = [&](const SemanticValues &vs, std::any &dt) {
      auto &data = *std::any_cast<Data *>(dt);

      if (vs.choice() == 2) {
        data.preamble = std::any_cast<std::string>(vs[1]);
        return;
      }

      auto is_macro = vs.choice() == 0;
      auto ignore = std::any_cast<bool>(vs[0]);
      auto name = std::any_cast<std::string>(vs[1]);
      auto enable_memoize = true;
      if (name.size() > NO_PACKRAT.size() &&
          name.substr(name.size() - NO_PACKRAT.size()) == NO_PACKRAT) {
        enable_memoize = false;
        name = name.substr(0, name.size() - NO_PACKRAT.size());
      }

      std::vector<std::string> params;
      std::shared_ptr<Ope> ope;
      if (is_macro) {
        params = std::any_cast<std::vector<std::string>>(vs[2]);
        ope = std::any_cast<std::shared_ptr<Ope>>(vs[4]);
        if (vs.size() == 6) {
          data.instructions[name] = std::any_cast<Instruction>(vs[5]);
        }
      } else {
        ope = std::any_cast<std::shared_ptr<Ope>>(vs[3]);
        if (vs.size() == 5) {
          data.instructions[name] = std::any_cast<Instruction>(vs[4]);
        }
      }

      auto &grammar = *data.grammar;
      if (!grammar.count(name)) {
        auto &rule = grammar[name];
        rule <= ope;
        rule.name = name;
        rule.s_ = vs.sv().data();
        rule.ignoreSemanticValue = ignore;
        rule.is_macro = is_macro;
        rule.params = params;
        rule.enable_memoize = enable_memoize;

        if (data.start.empty()) {
          data.start = name;
          data.start_pos = vs.sv().data();
        }
      } else {
        data.duplicates.emplace_back(name, vs.sv().data());
      }
    };

    g["Definition"].enter = [](const char * /*s*/, size_t /*n*/, std::any &dt) {
      auto &data = *std::any_cast<Data *>(dt);
      data.captures.clear();
    };

    auto exprFn = [&](const SemanticValues &vs) {
      if (vs.size() == 1) {
        return std::any_cast<std::shared_ptr<Ope>>(vs[0]);
      } else {
        std::vector<std::shared_ptr<Ope>> opes;
        for (auto i = 0u; i < vs.size(); i++) {
          opes.emplace_back(std::any_cast<std::shared_ptr<Ope>>(vs[i]));
        }
        const std::shared_ptr<Ope> ope = std::make_shared<PrioritizedChoice>(opes);
        return ope;
      }
    };
    g["Expression"] = exprFn;
    g["TopExpression"] = exprFn;
    g["TopChoice"] = [&](const SemanticValues &vs) {
      if (vs.size() > 1) {
        auto op = std::any_cast<std::shared_ptr<Ope>>(vs[0]);
        op->code = std::any_cast<std::string>(vs[1]);
      }
      return vs[0];
    };

    g["Sequence"] = [&](const SemanticValues &vs) {
      if (vs.empty()) {
        return npd(lit(""));
      } else if (vs.size() == 1) {
        return std::any_cast<std::shared_ptr<Ope>>(vs[0]);
      } else {
        std::vector<std::shared_ptr<Ope>> opes;
        for (const auto &x : vs) {
          opes.emplace_back(std::any_cast<std::shared_ptr<Ope>>(x));
        }
        const std::shared_ptr<Ope> ope = std::make_shared<Sequence>(opes);
        return ope;
      }
    };

    g["Prefix"] = [&](const SemanticValues &vs) {
      std::shared_ptr<Ope> ope;
      if (vs.size() == 1) {
        ope = std::any_cast<std::shared_ptr<Ope>>(vs[0]);
      } else {
        assert(vs.size() == 2);
        auto tok = std::any_cast<char>(vs[0]);
        ope = std::any_cast<std::shared_ptr<Ope>>(vs[1]);
        if (tok == '&') {
          ope = apd(ope);
        } else { // '!'
          ope = npd(ope);
        }
      }
      return ope;
    };

    g["SuffixWithLabel"] = [&](const SemanticValues &vs, std::any &dt) {
      auto ope = std::any_cast<std::shared_ptr<Ope>>(vs[0]);
      if (vs.size() == 1) {
        return ope;
      } else {
        assert(vs.size() == 2);
        auto &data = *std::any_cast<Data *>(dt);
        const auto &ident = std::any_cast<std::string>(vs[1]);
        auto label = ref(*data.grammar, ident, vs.sv().data(), false, {});
        auto recovery = rec(
            ref(*data.grammar, RECOVER_DEFINITION_NAME, vs.sv().data(), true, {label}));
        return cho4label_(ope, recovery);
      }
    };

    struct Loop {
      enum class Type { opt = 0, zom, oom, rep };
      Type type;
      std::pair<size_t, size_t> range;
    };

    g["Suffix"] = [&](const SemanticValues &vs) {
      auto ope = std::any_cast<std::shared_ptr<Ope>>(vs[0]);
      if (vs.size() == 1) {
        return ope;
      } else {
        assert(vs.size() == 2);
        auto loop = std::any_cast<Loop>(vs[1]);
        switch (loop.type) {
        case Loop::Type::opt:
          return opt(ope);
        case Loop::Type::zom:
          return zom(ope);
        case Loop::Type::oom:
          return oom(ope);
        default: // Regex-like repetition
          return rep(ope, loop.range.first, loop.range.second);
        }
      }
    };

    g["Loop"] = [&](const SemanticValues &vs) {
      switch (vs.choice()) {
      case 0: // Option
        return Loop{Loop::Type::opt, std::pair<size_t, size_t>()};
      case 1: // Zero or More
        return Loop{Loop::Type::zom, std::pair<size_t, size_t>()};
      case 2: // One or More
        return Loop{Loop::Type::oom, std::pair<size_t, size_t>()};
      default: // Regex-like repetition
        return Loop{Loop::Type::rep, std::any_cast<std::pair<size_t, size_t>>(vs[0])};
      }
    };

    g["RepetitionRange"] = [&](const SemanticValues &vs) {
      switch (vs.choice()) {
      case 0: { // Number COMMA Number
        auto min = std::any_cast<size_t>(vs[0]);
        auto max = std::any_cast<size_t>(vs[1]);
        return std::pair(min, max);
      }
      case 1: // Number COMMA
        return std::pair(std::any_cast<size_t>(vs[0]),
                         std::numeric_limits<size_t>::max());
      case 2: { // Number
        auto n = std::any_cast<size_t>(vs[0]);
        return std::pair(n, n);
      }
      default: // COMMA Number
        return std::pair(std::numeric_limits<size_t>::min(),
                         std::any_cast<size_t>(vs[0]));
      }
    };
    g["Number"] = [&](const SemanticValues &vs) {
      return vs.token_to_number<size_t>();
    };

    g["Primary"] = [&](const SemanticValues &vs, std::any &dt) {
      auto &data = *std::any_cast<Data *>(dt);

      switch (vs.choice()) {
      case 0:   // Macro Reference
      case 1: { // Reference
        auto is_macro = vs.choice() == 0;
        auto ignore = std::any_cast<bool>(vs[0]);
        const auto &ident = std::any_cast<std::string>(vs[1]);

        std::vector<std::shared_ptr<Ope>> args;
        if (is_macro) {
          args = std::any_cast<std::vector<std::shared_ptr<Ope>>>(vs[2]);
        }

        auto ope = ref(*data.grammar, ident, vs.sv().data(), is_macro, args);
        if (ident == RECOVER_DEFINITION_NAME) {
          ope = rec(ope);
        }

        if (ignore) {
          return ign(ope);
        } else {
          return ope;
        }
      }
      case 2: { // (Expression)
        return std::any_cast<std::shared_ptr<Ope>>(vs[0]);
      }
      case 3: { // TokenBoundary
        return tok(std::any_cast<std::shared_ptr<Ope>>(vs[0]));
      }
      case 4: { // CaptureScope
        return csc(std::any_cast<std::shared_ptr<Ope>>(vs[0]));
      }
      case 5: { // Capture
        const auto &name = std::any_cast<std::string_view>(vs[0]);
        auto ope = std::any_cast<std::shared_ptr<Ope>>(vs[1]);

        data.captures.insert(name);

        return cap(ope, [name](const char *a_s, size_t a_n, Context &c) {
          auto &cs = c.capture_scope_stack[c.capture_scope_stack_size - 1];
          cs[name] = std::string(a_s, a_n);
        });
      }
      default: {
        return std::any_cast<std::shared_ptr<Ope>>(vs[0]);
      }
      }
    };

    g["Identifier"] = [](const SemanticValues &vs) {
      string s = std::any_cast<std::string>(vs[0]);
      if (!vs.tokens.empty())
        s += vs.token_to_string();
      return s;
    };
    g["IdentCont"] = [](const SemanticValues &vs) {
      return std::string(vs.sv().data(), vs.sv().length());
    };

    g["Dictionary"] = [](const SemanticValues &vs) {
      auto items = vs.transform<std::string>();
      return dic(items);
    };

    g["Literal"] = [](const SemanticValues &vs) {
      const auto &tok = vs.tokens.front();
      return lit(resolve_escape_sequence(tok.data(), tok.size()));
    };
    g["LiteralI"] = [](const SemanticValues &vs) {
      const auto &tok = vs.tokens.front();
      return liti(resolve_escape_sequence(tok.data(), tok.size()));
    };
    g["LiteralD"] = [](const SemanticValues &vs) {
      auto &tok = vs.tokens.front();
      return resolve_escape_sequence(tok.data(), tok.size());
    };

    g["Class"] = [](const SemanticValues &vs) {
      auto ranges = vs.transform<std::pair<char32_t, char32_t>>();
      return cls(ranges);
    };
    g["NegatedClass"] = [](const SemanticValues &vs) {
      auto ranges = vs.transform<std::pair<char32_t, char32_t>>();
      return ncls(ranges);
    };
    g["Range"] = [](const SemanticValues &vs) {
      switch (vs.choice()) {
      case 0: {
        auto s1 = std::any_cast<std::string>(vs[0]);
        auto s2 = std::any_cast<std::string>(vs[1]);
        auto cp1 = decode_codepoint(s1.data(), s1.length());
        auto cp2 = decode_codepoint(s2.data(), s2.length());
        return std::pair(cp1, cp2);
      }
      case 1: {
        auto s = std::any_cast<std::string>(vs[0]);
        auto cp = decode_codepoint(s.data(), s.length());
        return std::pair(cp, cp);
      }
      }
      return std::pair<char32_t, char32_t>(0, 0);
    };
    g["Char"] = [](const SemanticValues &vs) {
      return resolve_escape_sequence(vs.sv().data(), vs.sv().length());
    };

    g["AND"] = [](const SemanticValues &vs) { return *vs.sv().data(); };
    g["NOT"] = [](const SemanticValues &vs) { return *vs.sv().data(); };
    g["QUESTION"] = [](const SemanticValues &vs) { return *vs.sv().data(); };
    g["STAR"] = [](const SemanticValues &vs) { return *vs.sv().data(); };
    g["PLUS"] = [](const SemanticValues &vs) { return *vs.sv().data(); };

    g["DOT"] = [](const SemanticValues & /*vs*/) { return dot(); };

    g["CUT"] = [](const SemanticValues & /*vs*/) { return cut(); };

    g["BeginCap"] = [](const SemanticValues &vs) { return vs.token(); };

    g["BackRef"] = [&](const SemanticValues &vs, std::any &dt) {
      auto &data = *std::any_cast<Data *>(dt);
      if (data.captures.find(vs.token()) == data.captures.end()) {
        data.enablePackratParsing = false;
      }
      return bkr(vs.token_to_string());
    };

    g["Ignore"] = [](const SemanticValues &vs) { return vs.size() > 0; };

    g["Parameters"] = [](const SemanticValues &vs) {
      return vs.transform<std::string>();
    };

    g["Arguments"] = [](const SemanticValues &vs) {
      return vs.transform<std::shared_ptr<Ope>>();
    };

    g["PrecedenceClimbing"] = [](const SemanticValues &vs) {
      PrecedenceClimbing::BinOpeInfo binOpeInfo;
      size_t level = 1;
      for (auto v : vs) {
        auto tokens = std::any_cast<std::vector<std::string_view>>(v);
        auto assoc = tokens[0][0];
        for (size_t i = 1; i < tokens.size(); i++) {
          binOpeInfo[tokens[i]] = std::pair(level, assoc);
        }
        level++;
      }
      Instruction instruction;
      instruction.type = "precedence";
      instruction.data = binOpeInfo;
      return instruction;
    };
    g["PrecedenceInfo"] = [](const SemanticValues &vs) {
      return vs.transform<std::string_view>();
    };
    g["PrecedenceOpe"] = [](const SemanticValues &vs) { return vs.token(); };
    g["PrecedenceAssoc"] = [](const SemanticValues &vs) { return vs.token(); };

    g["ErrorMessage"] = [](const SemanticValues &vs) {
      Instruction instruction;
      instruction.type = "message";
      instruction.data = std::any_cast<std::string>(vs[0]);
      return instruction;
    };

    g["CppCode"] = [](const SemanticValues &vs) { return std::string(vs.sv()); };

    g["NoAstOpt"] = [](const SemanticValues & /*vs*/) {
      Instruction instruction;
      instruction.type = "no_ast_opt";
      return instruction;
    };
  }

  bool apply_precedence_instruction(Definition &rule,
                                    const PrecedenceClimbing::BinOpeInfo &info,
                                    const char *s, Log log) {
    try {
      auto &seq = dynamic_cast<Sequence &>(*rule.get_core_operator());
      auto atom = seq.opes_[0];
      auto &rep = dynamic_cast<Repetition &>(*seq.opes_[1]);
      auto &seq1 = dynamic_cast<Sequence &>(*rep.ope_);
      auto binop = seq1.opes_[0];
      auto atom1 = seq1.opes_[1];

      auto atom_name = dynamic_cast<Reference &>(*atom).name_;
      auto binop_name = dynamic_cast<Reference &>(*binop).name_;
      auto atom1_name = dynamic_cast<Reference &>(*atom1).name_;

      if (!rep.is_zom() || atom_name != atom1_name || atom_name == binop_name) {
        if (log) {
          auto line = line_info(s, rule.s_);
          log(line.first, line.second,
              "'precedence' instruction cannot be applied to '" + rule.name + "'.");
        }
        return false;
      }

      rule.holder_->ope_ = pre(atom, binop, info, rule);
      rule.disable_action = true;
    } catch (...) {
      if (log) {
        auto line = line_info(s, rule.s_);
        log(line.first, line.second,
            "'precedence' instruction cannot be applied to '" + rule.name + "'.");
      }
      return false;
    }
    return true;
  }

public:
  std::shared_ptr<Grammar> perform_core(const char *s, size_t n, const Rules &rules,
                                        std::string &start, bool &enablePackratParsing,
                                        std::string &preamble, Log log) {
    Data data;
    auto &grammar = *data.grammar;

    // Built-in macros
    {
      // `%recover`
      {
        auto &rule = grammar[RECOVER_DEFINITION_NAME];
        rule <= ref(grammar, "x", "", false, {});
        rule.name = RECOVER_DEFINITION_NAME;
        rule.s_ = "[native]";
        rule.ignoreSemanticValue = true;
        rule.is_macro = true;
        rule.params = {"x"};
      }
    }

    std::any dt = &data;
    auto r = g["Grammar"].parse(s, n, dt, nullptr, log);

    if (!r.ret) {
      if (log) {
        if (r.error_info.message_pos) {
          auto line = line_info(s, r.error_info.message_pos);
          log(line.first, line.second, r.error_info.message);
        } else {
          auto line = line_info(s, r.error_info.error_pos);
          log(line.first, line.second, "syntax error");
        }
      }
      return nullptr;
    }

    // User provided rules
    for (auto [user_name, user_rule] : rules) {
      auto name = user_name;
      auto ignore = false;
      if (!name.empty() && name[0] == '~') {
        ignore = true;
        name.erase(0, 1);
      }
      if (!name.empty()) {
        auto &rule = grammar[name];
        rule <= user_rule;
        rule.name = name;
        rule.ignoreSemanticValue = ignore;
      }
    }

    // Check duplicated definitions
    auto ret = data.duplicates.empty();

    for (const auto &[name, ptr] : data.duplicates) {
      if (log) {
        auto line = line_info(s, ptr);
        log(line.first, line.second, "'" + name + "' is already defined.");
      }
    }

    // Set root definition
    auto &start_rule = grammar[data.start];

    // Check if the start rule has ignore operator
    {
      if (start_rule.ignoreSemanticValue) {
        if (log) {
          auto line = line_info(s, start_rule.s_);
          log(line.first, line.second,
              "Ignore operator cannot be applied to '" + start_rule.name + "'.");
        }
        ret = false;
      }
    }

    if (!ret) {
      return nullptr;
    }

    // Check missing definitions
    auto referenced = std::unordered_set<std::string>{
        WHITESPACE_DEFINITION_NAME, WORD_DEFINITION_NAME, RECOVER_DEFINITION_NAME,
        start_rule.name, "fstring"};

    for (auto &[_, rule] : grammar) {
      ReferenceChecker vis(grammar, rule.params);
      rule.accept(vis);
      referenced.insert(vis.referenced.begin(), vis.referenced.end());
      for (const auto &[name, ptr] : vis.error_s) {
        if (log) {
          auto line = line_info(s, ptr);
          log(line.first, line.second, vis.error_message[name]);
        }
        ret = false;
      }
    }

    for (auto &[name, rule] : grammar) {
      if (!referenced.count(name)) {
        if (log) {
          auto line = line_info(s, rule.s_);
          auto msg = "'" + name + "' is not referenced.";
          log(line.first, line.second, msg);
        }
      }
    }

    if (!ret) {
      return nullptr;
    }

    // Link references
    for (auto &x : grammar) {
      auto &rule = x.second;
      LinkReferences vis(grammar, rule.params);
      rule.accept(vis);
    }

    // Check left recursion
    ret = true;

    for (auto &[name, rule] : grammar) {
      DetectLeftRecursion vis(name);
      rule.accept(vis);
      if (vis.error_s) {
        if (log) {
          auto line = line_info(s, vis.error_s);
          log(line.first, line.second, "'" + name + "' is left recursive.");
        }
        ret = false;
      }
    }

    if (!ret) {
      return nullptr;
    }

    // Check infinite loop
    {
      DetectInfiniteLoop vis(data.start_pos, data.start);
      start_rule.accept(vis);
      if (vis.has_error) {
        if (log) {
          auto line = line_info(s, vis.error_s);
          log(line.first, line.second,
              "infinite loop is detected in '" + vis.error_name + "'.");
        }
        return nullptr;
      }
    }

    // Automatic whitespace skipping
    if (grammar.count(WHITESPACE_DEFINITION_NAME)) {
      for (auto &x : grammar) {
        auto &rule = x.second;
        auto ope = rule.get_core_operator();
        if (IsLiteralToken::check(*ope)) {
          rule <= tok(ope);
        }
      }

      start_rule.whitespaceOpe =
          wsp(grammar[WHITESPACE_DEFINITION_NAME].get_core_operator());
    }

    // Word expression
    if (grammar.count(WORD_DEFINITION_NAME)) {
      start_rule.wordOpe = grammar[WORD_DEFINITION_NAME].get_core_operator();
    }

    // Apply instructions
    for (const auto &[name, instruction] : data.instructions) {
      auto &rule = grammar[name];
      if (instruction.type == "precedence") {
        const auto &info =
            std::any_cast<PrecedenceClimbing::BinOpeInfo>(instruction.data);

        if (!apply_precedence_instruction(rule, info, s, log)) {
          return nullptr;
        }
      } else if (instruction.type == "message") {
        rule.error_message = std::any_cast<std::string>(instruction.data);
      } else if (instruction.type == "no_ast_opt") {
        rule.no_ast_opt = true;
      }
    }

    // Disable packrat on demand
    unordered_set<string> seen;
    for (auto &[name, rule] : grammar) {
      auto packrat = SetUpPackrat::check(&seen, rule.get_core_operator());
      if (!packrat)
        rule.enable_memoize = false;
    }

    // Set root definition
    start = data.start;
    enablePackratParsing = data.enablePackratParsing;
    preamble = data.preamble;
    return data.grammar;
  }

  Grammar g;
};

class PrintVisitor : public Ope::Visitor {
  vector<string> v;

public:
  static string parse(const shared_ptr<Ope> &op) {
    PrintVisitor v;
    op->accept(v);
    if (v.v.size()) {
      if (v.v[0].empty())
        return fmt::format("P[\"{}\"]", v.v[1]);
      else
        return fmt::format("{}({})", v.v[0], join(v.v, ", ", 1));
    }
    return "-";
  };

private:
  void visit(Sequence &s) override {
    v = {"seq"};
    for (auto &o : s.opes_)
      v.push_back(parse(o));
  }
  void visit(PrioritizedChoice &s) override {
    v = {"cho"};
    for (auto &o : s.opes_)
      v.push_back(parse(o));
  }
  void visit(Repetition &s) override {
    if (s.is_zom())
      v = {"zom", parse(s.ope_)};
    else if (s.min_ == 1 && s.max_ == std::numeric_limits<size_t>::max())
      v = {"oom", parse(s.ope_)};
    else if (s.min_ == 0 && s.max_ == 1)
      v = {"opt", parse(s.ope_)};
    else
      v = {"rep", parse(s.ope_), to_string(s.min_), to_string(s.max_)};
  }
  void visit(AndPredicate &s) override { v = {"apd", parse(s.ope_)}; }
  void visit(NotPredicate &s) override { v = {"npd", parse(s.ope_)}; }
  void visit(LiteralString &s) override {
    v = {s.ignore_case_ ? "liti" : "lit", fmt::format("\"{}\"", escape(s.lit_))};
  }
  void visit(CharacterClass &s) override {
    vector<string> sv;
    for (auto &c : s.ranges_)
      sv.push_back(fmt::format("{{0x{:x}, 0x{:x}}}", (int)c.first, (int)c.second));
    v = {s.negated_ ? "ncls" : "cls", "vc{" + join(sv, ",") + "}"};
  }
  void visit(Character &s) override { v = {"chr", fmt::format("'{}'", s.ch_)}; }
  void visit(AnyCharacter &s) override { v = {"dot"}; }
  void visit(Cut &s) override { v = {"cut"}; }
  void visit(Reference &s) override {
    if (s.is_macro_) {
      vector<string> vs;
      for (auto &o : s.args_)
        vs.push_back(parse(o));
      v = {"ref",  "P",    fmt::format("\"{}\"", s.name_),
           "\"\"", "true", "{" + join(vs, ", ") + "}"};
    } else {
      v = {"ref", "P", fmt::format("\"{}\"", s.name_)};
    }
  }
  void visit(TokenBoundary &s) override { v = {"tok", parse(s.ope_)}; }
  void visit(Ignore &s) override { v = {"ign", parse(s.ope_)}; }
  void visit(Recovery &s) override { v = {"rec", parse(s.ope_)}; }
  // infix TODO
};

} // namespace peg

int main(int argc, char **argv) {
  peg::parser parser;
  fmt::print("Generating grammar from {}\n", argv[1]);
  ifstream ifs(argv[1]);
  string g((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
  ifs.close();

  string start;
  peg::Rules dummy = {};
  if (string(argv[3]) == "seq")
    dummy["NLP"] = peg::usr([](const char *, size_t, peg::SemanticValues &,
                               any &) -> size_t { return -1; });
  bool enablePackratParsing;
  string preamble;
  peg::Log log = [](size_t line, size_t col, const string &msg) {
    cerr << line << ":" << col << ": " << msg << "\n";
  };
  auto grammar = peg::ParserGenerator::get_instance().perform_core(
      g.c_str(), g.size(), dummy, start, enablePackratParsing, preamble, log);
  assert(grammar);

  string rules, actions;
  string action_preamble = "  auto &CTX = any_cast<ParseContext &>(DT);\n";
  string loc_preamble = "  auto LI = VS.line_info();\n"
                        "  auto LOC = seq::SrcInfo(\n"
                        "    VS.path, LI.first + CTX.line_offset,\n"
                        "    LI.second + CTX.col_offset,\n"
                        "    VS.sv().size());\n";

  for (auto &[name, def] : *grammar) {
    auto op = def.get_core_operator();
    if (dummy.find(name) != dummy.end())
      continue;
    rules += fmt::format("  {}P[\"{}\"] <= {};\n", def.ignoreSemanticValue ? "~" : "",
                         name, peg::PrintVisitor::parse(op));
    rules += fmt::format("  P[\"{}\"].name = \"{}\";\n", name, escape(name));
    if (def.is_macro)
      rules += fmt::format("  P[\"{}\"].is_macro = true;\n", name);
    if (!def.enable_memoize)
      rules += fmt::format("  P[\"{}\"].enable_memoize = false;\n", name);
    if (!def.params.empty()) {
      vector<string> params;
      for (auto &p : def.params)
        params.push_back(fmt::format("\"{}\"", escape(p)));
      rules += fmt::format("  P[\"{}\"].params = {{{}}};\n", name, join(params, ", "));
    }

    string code = op->code;
    if (code.empty()) {
      bool all_empty = true;
      if (auto ope = dynamic_cast<peg::PrioritizedChoice *>(op.get())) {
        for (int i = 0; i < ope->opes_.size(); i++)
          if (!ope->opes_[i]->code.empty()) {
            code +=
                fmt::format("  if (VS.choice() == {}) {}\n", i, ope->opes_[i]->code);
            all_empty = false;
          } else {
            code += fmt::format("  if (VS.choice() == {}) return V0;\n", i);
          }
      }
      if (all_empty)
        code = "";
      if (!code.empty())
        code = "{\n" + code + "}";
    }
    if (!code.empty()) {
      code = code.substr(1, code.size() - 2);
      if (code.find("LOC") != std::string::npos)
        code = loc_preamble + code;
      if (code.find("CTX") != std::string::npos)
        code = action_preamble + code;
      actions += fmt::format(
          "P[\"{}\"] = [](peg::SemanticValues &VS, any &DT) {{\n{}\n}};\n", name, code);
    }
  };

  FILE *fout = fopen(argv[2], "w");
  fmt::print(fout, "// clang-format off\n");
  fmt::print(fout, "#pragma clang diagnostic push\n");
  fmt::print(fout, "#pragma clang diagnostic ignored \"-Wreturn-type\"\n");
  if (!preamble.empty())
    fmt::print(fout, "{}\n", preamble.substr(1, preamble.size() - 2));
  string rules_preamble = "  using namespace peg;\n"
                          "  using peg::seq;\n"
                          "  using vc = vector<pair<char32_t, char32_t>>;\n";
  fmt::print(fout, "void init_{}_rules(peg::Grammar &P) {{\n{}\n{}\n}}\n", argv[3],
             rules_preamble, rules);
  string act;
  for (auto &c : actions)
    if (c == '\n')
      act += "\n  ";
    else
      act += c;
  fmt::print(fout, "void init_{}_actions(peg::Grammar &P) {{\n  {}\n}}\n", argv[3],
             act);
  fmt::print(fout, "// clang-format on\n");
  fmt::print(fout, "#pragma clang diagnostic pop\n");
  fclose(fout);

  return 0;
}
