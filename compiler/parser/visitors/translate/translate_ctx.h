/*
 * translate_ctx.h --- Context for IR translation stage.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "parser/cache.h"
#include "parser/common.h"
#include "parser/ctx.h"
#include "sir/sir.h"
#include "sir/types/types.h"

namespace seq {
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
    seq::ir::Var *var;
    seq::ir::Func *func;
    seq::ir::types::Type *type;
  } handle;
  /// Base function pointer.
  seq::ir::BodiedFunc *base;

  TranslateItem(Kind k, seq::ir::BodiedFunc *base)
      : kind(k), handle{nullptr}, base(base) {}
  const seq::ir::BodiedFunc *getBase() const { return base; }
  seq::ir::Func *getFunc() const { return kind == Func ? handle.func : nullptr; }
  seq::ir::types::Type *getType() const { return kind == Type ? handle.type : nullptr; }
  seq::ir::Var *getVar() const { return kind == Var ? handle.var : nullptr; }
};

/**
 * A variable table (context) for the IR translation stage.
 */
struct TranslateContext : public Context<TranslateItem> {
  /// A pointer to the shared cache.
  shared_ptr<Cache> cache;
  /// Stack of function bases.
  vector<seq::ir::BodiedFunc *> bases;
  /// Stack of IR series (blocks).
  vector<seq::ir::SeriesFlow *> series;

public:
  TranslateContext(shared_ptr<Cache> cache, seq::ir::SeriesFlow *series,
                   seq::ir::BodiedFunc *base);

  using Context<TranslateItem>::add;
  /// Convenience method for adding an object to the context.
  shared_ptr<TranslateItem> add(TranslateItem::Kind kind, const string &name,
                                void *type);
  shared_ptr<TranslateItem> find(const string &name) const override;

  /// Convenience method for adding a series.
  void addSeries(seq::ir::SeriesFlow *s);
  void popSeries();

public:
  seq::ir::Module *getModule() const;
  seq::ir::BodiedFunc *getBase() const;
  seq::ir::SeriesFlow *getSeries() const;
};

} // namespace ast
} // namespace seq
