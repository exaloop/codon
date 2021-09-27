/*
 * translate_ctx.cpp --- Context for IR translation stage.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#include <memory>
#include <vector>

#include "parser/common.h"
#include "parser/ctx.h"
#include "parser/visitors/translate/translate.h"
#include "parser/visitors/translate/translate_ctx.h"
#include "parser/visitors/typecheck/typecheck_ctx.h"

namespace seq {
namespace ast {

TranslateContext::TranslateContext(shared_ptr<Cache> cache, seq::ir::SeriesFlow *series,
                                   seq::ir::BodiedFunc *base)
    : Context<TranslateItem>(""), cache(std::move(cache)) {
  stack.push_front(vector<string>());
  bases.push_back(base);
  addSeries(series);
}

shared_ptr<TranslateItem> TranslateContext::find(const string &name) const {
  if (auto t = Context<TranslateItem>::find(name))
    return t;
  shared_ptr<TranslateItem> ret = nullptr;
  auto tt = cache->typeCtx->find(name);
  if (tt->isType() && tt->type->canRealize()) {
    ret = make_shared<TranslateItem>(TranslateItem::Type, bases[0]);
    seqassert(in(cache->classes, tt->type->getClass()->name) &&
                  in(cache->classes[tt->type->getClass()->name].realizations, name),
              "cannot find type realization {}", name);
    ret->handle.type =
        cache->classes[tt->type->getClass()->name].realizations[name]->ir;
  } else if (tt->type->getFunc() && tt->type->canRealize()) {
    ret = make_shared<TranslateItem>(TranslateItem::Func, bases[0]);
    seqassert(
        in(cache->functions, tt->type->getFunc()->ast->name) &&
            in(cache->functions[tt->type->getFunc()->ast->name].realizations, name),
        "cannot find type realization {}", name);
    ret->handle.func =
        cache->functions[tt->type->getFunc()->ast->name].realizations[name]->ir;
  }
  return ret;
}

shared_ptr<TranslateItem> TranslateContext::add(TranslateItem::Kind kind,
                                                const string &name, void *type) {
  auto it = make_shared<TranslateItem>(kind, getBase());
  if (kind == TranslateItem::Var)
    it->handle.var = (ir::Var *)type;
  else if (kind == TranslateItem::Func)
    it->handle.func = (ir::Func *)type;
  else
    it->handle.type = (ir::types::Type *)type;
  add(name, it);
  return it;
}

void TranslateContext::addSeries(seq::ir::SeriesFlow *s) { series.push_back(s); }
void TranslateContext::popSeries() { series.pop_back(); }

seq::ir::Module *TranslateContext::getModule() const {
  return dynamic_cast<seq::ir::Module *>(bases[0]->getModule());
}
seq::ir::BodiedFunc *TranslateContext::getBase() const { return bases.back(); }
seq::ir::SeriesFlow *TranslateContext::getSeries() const { return series.back(); }

} // namespace ast
} // namespace seq
