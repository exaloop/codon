// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "replacer.h"

#include <unordered_set>

#include "codon/cir/types/types.h"
#include "codon/cir/value.h"
#include "codon/cir/var.h"

namespace codon {
namespace ir {
namespace transform {
namespace cleanup {

const std::string ReplaceCleanupPass::KEY = "core-cleanup-physical-replace";

void ReplaceCleanupPass::run(Module *module) {
  std::unordered_set<Value *> valuesToDelete;
  std::unordered_set<types::Type *> typesToDelete;
  std::unordered_set<Var *> varsToDelete;

  {
    auto *f = module->getMainFunc();
    for (auto *c : f->getUsedValues()) {
      if (c->hasReplacement()) {
        f->replaceUsedValue(c, c->getActual());
        valuesToDelete.insert(c);
      }
    }
    for (auto *t : f->getUsedTypes()) {
      if (t->hasReplacement()) {
        f->replaceUsedType(t, t->getActual());
        typesToDelete.insert(t);
      }
    }
    for (auto *v : f->getUsedVariables()) {
      if (v->hasReplacement()) {
        f->replaceUsedVariable(v, v->getActual());
        varsToDelete.insert(v);
      }
    }
  }

  {
    auto *v = module->getArgVar();
    for (auto *c : v->getUsedValues()) {
      if (c->hasReplacement()) {
        v->replaceUsedValue(c, c->getActual());
        valuesToDelete.insert(c);
      }
    }
    for (auto *t : v->getUsedTypes()) {
      if (t->hasReplacement()) {
        v->replaceUsedType(t, t->getActual());
        typesToDelete.insert(t);
      }
    }
    for (auto *v2 : v->getUsedVariables()) {
      if (v2->hasReplacement()) {
        v->replaceUsedVariable(v2, v2->getActual());
        varsToDelete.insert(v2);
      }
    }
  }

  for (auto it = module->values_begin(); it != module->values_end(); ++it) {
    for (auto *c : it->getUsedValues()) {
      if (c->hasReplacement()) {
        it->replaceUsedValue(c, c->getActual());
        valuesToDelete.insert(c);
      }
    }
    for (auto *t : it->getUsedTypes()) {
      if (t->hasReplacement()) {
        it->replaceUsedType(t, t->getActual());
        typesToDelete.insert(t);
      }
    }
    for (auto *v : it->getUsedVariables()) {
      if (v->hasReplacement()) {
        it->replaceUsedVariable(v, v->getActual());
        varsToDelete.insert(v);
      }
    }
  }

  for (auto it = module->begin(); it != module->end(); ++it) {
    for (auto *c : it->getUsedValues()) {
      if (c->hasReplacement()) {
        it->replaceUsedValue(c, c->getActual());
        valuesToDelete.insert(c);
      }
    }
    for (auto *t : it->getUsedTypes()) {
      if (t->hasReplacement()) {
        it->replaceUsedType(t, t->getActual());
        typesToDelete.insert(t);
      }
    }
    for (auto *v : it->getUsedVariables()) {
      if (v->hasReplacement()) {
        it->replaceUsedVariable(v, v->getActual());
        varsToDelete.insert(v);
      }
    }
  }

  for (auto it = module->types_begin(); it != module->types_end(); ++it) {
    for (auto *c : it->getUsedValues()) {
      if (c->hasReplacement()) {
        it->replaceUsedValue(c, c->getActual());
        valuesToDelete.insert(c);
      }
    }
    for (auto *t : it->getUsedTypes()) {
      if (t->hasReplacement()) {
        it->replaceUsedType(t, t->getActual());
        typesToDelete.insert(t);
      }
    }
    for (auto *v : it->getUsedVariables()) {
      if (v->hasReplacement()) {
        it->replaceUsedVariable(v, v->getActual());
        varsToDelete.insert(v);
      }
    }
  }

  for (auto *v : valuesToDelete)
    module->remove(v);
  for (auto *v : varsToDelete)
    module->remove(v);
  for (auto *t : typesToDelete)
    module->remove(t);
}

} // namespace cleanup
} // namespace transform
} // namespace ir
} // namespace codon
