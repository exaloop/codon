// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "translate_ctx.h"

#include <memory>
#include <vector>

#include "codon/parser/common.h"
#include "codon/parser/ctx.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/ctx.h"

namespace codon::ast {

TranslateContext::TranslateContext(Cache *cache)
    : Context<TranslateItem>(""), cache(cache) {}

std::shared_ptr<TranslateItem> TranslateContext::find(const std::string &name) const {
  if (auto t = Context<TranslateItem>::find(name))
    return t;
  std::shared_ptr<TranslateItem> ret = nullptr;
  auto tt = cache->typeCtx->find(name);
  if (tt && tt->isType() && tt->type->canRealize()) {
    ret = std::make_shared<TranslateItem>(TranslateItem::Type, bases[0]);
    seqassertn(in(cache->classes, tt->type->getClass()->name) &&
                   in(cache->classes[tt->type->getClass()->name].realizations, name),
               "cannot find type realization {}", name);
    ret->handle.type =
        cache->classes[tt->type->getClass()->name].realizations[name]->ir;
  } else if (tt && tt->type->getFunc() && tt->type->canRealize()) {
    ret = std::make_shared<TranslateItem>(TranslateItem::Func, bases[0]);
    seqassertn(
        in(cache->functions, tt->type->getFunc()->ast->name) &&
            in(cache->functions[tt->type->getFunc()->ast->name].realizations, name),
        "cannot find type realization {}", name);
    ret->handle.func =
        cache->functions[tt->type->getFunc()->ast->name].realizations[name]->ir;
  }
  return ret;
}

std::shared_ptr<TranslateItem>
TranslateContext::forceFind(const std::string &name) const {
  auto i = find(name);
  seqassertn(i, "cannot find '{}'", name);
  return i;
}

std::shared_ptr<TranslateItem>
TranslateContext::add(TranslateItem::Kind kind, const std::string &name, void *type) {
  auto it = std::make_shared<TranslateItem>(kind, getBase());
  if (kind == TranslateItem::Var)
    it->handle.var = (ir::Var *)type;
  else if (kind == TranslateItem::Func)
    it->handle.func = (ir::Func *)type;
  else
    it->handle.type = (ir::types::Type *)type;
  add(name, it);
  return it;
}

void TranslateContext::addSeries(codon::ir::SeriesFlow *s) { series.push_back(s); }
void TranslateContext::popSeries() { series.pop_back(); }

codon::ir::Module *TranslateContext::getModule() const {
  return dynamic_cast<codon::ir::Module *>(bases[0]->getModule());
}
codon::ir::BodiedFunc *TranslateContext::getBase() const { return bases.back(); }
codon::ir::SeriesFlow *TranslateContext::getSeries() const { return series.back(); }

} // namespace codon::ast
