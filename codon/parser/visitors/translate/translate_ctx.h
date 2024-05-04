// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "codon/cir/cir.h"
#include "codon/cir/types/types.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/ctx.h"

namespace codon::ast {

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
  Cache *cache;
  /// Stack of function bases.
  std::vector<codon::ir::BodiedFunc *> bases;
  /// Stack of IR series (blocks).
  std::vector<codon::ir::SeriesFlow *> series;
  /// Stack of sequence items for attribute initialization.
  std::vector<std::vector<std::pair<ExprAttr, ir::Value *>>> seqItems;

public:
  TranslateContext(Cache *cache);

  using Context<TranslateItem>::add;
  /// Convenience method for adding an object to the context.
  std::shared_ptr<TranslateItem> add(TranslateItem::Kind kind, const std::string &name,
                                     void *type);
  std::shared_ptr<TranslateItem> find(const std::string &name) const override;
  std::shared_ptr<TranslateItem> forceFind(const std::string &name) const;

  /// Convenience method for adding a series.
  void addSeries(codon::ir::SeriesFlow *s);
  void popSeries();

public:
  codon::ir::Module *getModule() const;
  codon::ir::BodiedFunc *getBase() const;
  codon::ir::SeriesFlow *getSeries() const;
};

} // namespace codon::ast
