#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/ctx.h"
#include "codon/sir/sir.h"
#include "codon/sir/types/types.h"

namespace codon {
namespace ast {

/**
 * IR context object description.
 * This represents an identifier that can be either a function, a class (type), or a
 * variable.
 */
struct TranslateItem {
  enum Kind { Func, Type, Var } kind;
  /// IR handle.
  union {
    codon::ir::Var *var;
    codon::ir::Func *func;
    codon::ir::types::Type *type;
  } handle;
  /// Base function pointer.
  codon::ir::BodiedFunc *base;

  TranslateItem(Kind k, codon::ir::BodiedFunc *base)
      : kind(k), handle{nullptr}, base(base) {}
  const codon::ir::BodiedFunc *getBase() const { return base; }
  codon::ir::Func *getFunc() const { return kind == Func ? handle.func : nullptr; }
  codon::ir::types::Type *getType() const {
    return kind == Type ? handle.type : nullptr;
  }
  codon::ir::Var *getVar() const { return kind == Var ? handle.var : nullptr; }
};

/**
 * A variable table (context) for the IR translation stage.
 */
struct TranslateContext : public Context<TranslateItem> {
  /// A pointer to the shared cache.
  shared_ptr<Cache> cache;
  /// Stack of function bases.
  vector<codon::ir::BodiedFunc *> bases;
  /// Stack of IR series (blocks).
  vector<codon::ir::SeriesFlow *> series;

public:
  TranslateContext(shared_ptr<Cache> cache, codon::ir::SeriesFlow *series,
                   codon::ir::BodiedFunc *base);

  using Context<TranslateItem>::add;
  /// Convenience method for adding an object to the context.
  shared_ptr<TranslateItem> add(TranslateItem::Kind kind, const string &name,
                                void *type);
  shared_ptr<TranslateItem> find(const string &name) const override;

  /// Convenience method for adding a series.
  void addSeries(codon::ir::SeriesFlow *s);
  void popSeries();

public:
  codon::ir::Module *getModule() const;
  codon::ir::BodiedFunc *getBase() const;
  codon::ir::SeriesFlow *getSeries() const;
};

} // namespace ast
} // namespace codon
