// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "devirtualize.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>

#include "codon/cir/analyze/dataflow/reaching.h"
#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace cleanup {
namespace {

// Bound proof search separately from code growth. Exhausting either budget may
// lose an optimization, but must never turn an unknown type into an exact one.
constexpr size_t MAX_TYPE_DEPTH = 256;
constexpr size_t MAX_SUMMARY_DEPTH = 32;
constexpr size_t MAX_ARGUMENT_FIELDS = 64;
constexpr size_t MAX_SPECIALIZATION_SIZE = 256;
constexpr size_t MAX_SPECIALIZATION_GROWTH = 32768;
constexpr size_t MAX_VARIANTS = 8;
const std::string SPECIALIZED_ATTR = ".devirtualized";

// A query path projects through zero-based record fields. Argument facts prefix
// that path with an argument index: {1, 0} is the first field of the second arg.
using Path = std::vector<int>;
using Facts = std::map<Path, RefType *>;
// Contexts are shared by all calls with the same source function and type facts;
// individual value queries additionally depend on the projected source value.
using ContextKey = std::pair<id_t, std::vector<std::pair<Path, std::string>>>;
using Query = std::pair<id_t, Path>;

// Inspect the final callee without discarding its FlowInstr setup. Rewrites must
// still evaluate that setup even when they replace the resulting function value.
Func *directCallee(const Value *value) {
  while (auto *flow = cast<FlowInstr>(value))
    value = flow->getValue();
  return util::getFunc(const_cast<Value *>(value));
}

// Context-independent source-body inventory, reused for summaries, rewrite
// planning, and the specialization size estimate.
struct BodyInfo : public util::Operator {
  size_t size = 0;
  bool hasSharedValues = false;
  llvm::DenseSet<id_t> operands;
  std::vector<Value *> returns;
  std::vector<CallInstr *> calls;
  std::vector<std::pair<Value *, Value *>> fields;
  void preHook(Node *node) override {
    ++size;
    if (hasSharedValues)
      return;
    for (auto *value : node->getUsedValues()) {
      if (value && !isA<Const>(value) && !util::getFunc(value) &&
          !operands.insert(value->getId()).second) {
        hasSharedValues = true;
        break;
      }
    }
  }
  void handle(ReturnInstr *value) override { returns.push_back(value->getValue()); }
  void handle(CallInstr *value) override { calls.push_back(value); }
  void handle(ExtractInstr *value) override {
    fields.emplace_back(value, value->getVal());
  }
  void handle(InsertInstr *value) override {
    fields.emplace_back(value, value->getLhs());
  }
};

// Analyze the original body under one set of exact argument facts. An empty set
// describes the ordinary function; nonempty facts justify a private target clone.
// RD queries always use source nodes, while planned edits name target nodes.
struct Context {
  BodiedFunc *source;
  BodiedFunc *target;
  Facts arguments;
  std::unique_ptr<util::CloneVisitor> clone;
  // Memoized value/projection proofs and return/projection summaries. The active
  // sets break cycles conservatively rather than assuming a recursive result.
  std::map<Query, RefType *> cache;
  std::set<Query> active;
  std::map<Path, RefType *> returns;
  std::set<Path> activeReturns;
  // Field annotations, specialized callees, and resolved virtual-lookup results.
  std::vector<std::pair<Value *, RefType *>> fields;
  std::vector<std::pair<CallInstr *, Func *>> calls;
  std::vector<std::pair<CallInstr *, Func *>> lookups;

  Context(BodiedFunc *source, Facts arguments)
      : source(source), target(source), arguments(std::move(arguments)) {}
};

// Discover contexts and plan all edits against immutable source bodies, then
// apply them together. This keeps the existing CFG/RD analysis valid throughout
// proof search. Specialization exposes exact types; LLVM still decides inlining.
class Specializer {
  Module *module;
  analyze::dataflow::RDResult &reaching;
  bool jit;
  size_t growth = 0;
  size_t summaryDepth = 0;
  llvm::DenseMap<id_t, std::unique_ptr<BodyInfo>> bodies;
  llvm::DenseMap<id_t, size_t> variants;
  std::map<ContextKey, std::unique_ptr<Context>> contexts;
  std::vector<Context *> pending;

  BodyInfo &body(BodiedFunc *function) {
    auto &info = bodies[function->getId()];
    if (!info) {
      info = std::make_unique<BodyInfo>();
      info->process(function->getBody());
    }
    return *info;
  }

  // Reuse a context or create a bounded specialization. On budget exhaustion,
  // analyze the original without argument assumptions and keep calls unspecialized.
  Context *context(BodiedFunc *function, Facts arguments = {}) {
    if (!function || function->isGenerator() || function->isAsync() ||
        !reaching.results.count(function->getId()) ||
        util::hasAttribute(function, SPECIALIZED_ATTR))
      return nullptr;
    ContextKey key{function->getId(), {}};
    for (const auto &fact : arguments)
      key.second.emplace_back(fact.first, fact.second->getName());
    auto found = contexts.find(key);
    if (found != contexts.end())
      return found->second.get();
    if (!arguments.empty() &&
        (body(function).hasSharedValues ||
         variants[function->getId()] >= MAX_VARIANTS ||
         body(function).size > MAX_SPECIALIZATION_SIZE ||
         growth + body(function).size > MAX_SPECIALIZATION_GROWTH ||
         util::isGPUKernel(function)))
      return context(function);
    auto entry = std::make_unique<Context>(function, std::move(arguments));
    auto *result = entry.get();
    // Publish before processing calls so recursion can find this same context.
    contexts.emplace(std::move(key), std::move(entry));
    pending.push_back(result);
    if (!result->arguments.empty()) {
      ++variants[function->getId()];
      growth += body(function).size;
      result->clone = std::make_unique<util::CloneVisitor>(module);
      auto &clone = *result->clone;
      auto *target =
          module->N<BodiedFunc>(function, function->getName() + ".devirtualized");
      result->target = target;
      target->setUnmangledName(function->getUnmangledName());
      target->setParentType(function->getParentType());
      std::vector<std::string> names;
      for (auto argument = function->arg_begin(); argument != function->arg_end();
           ++argument)
        names.push_back((*argument)->getName());
      target->realize(cast<FuncType>(function->getType()), names);
      auto targetArgument = target->arg_begin();
      for (auto argument = function->arg_begin(); argument != function->arg_end();
           ++argument)
        clone.forceRemap(*argument, *targetArgument++);
      for (auto *variable : *function)
        target->push_back(clone.forceClone(variable));
      // Remap locals and parameters, not the source function itself. A recursive
      // call may pass different runtime types and needs its own context proof.
      target->setBody(cast<Flow>(clone.clone(function->getBody())));
      for (auto attribute = function->attributes_begin();
           attribute != function->attributes_end(); ++attribute) {
        const auto *value = function->getAttribute(*attribute);
        if (value->needsClone())
          target->setAttribute(value->clone(clone), *attribute);
      }
      auto *attributes = target->getAttribute<KeyValueAttribute>();
      if (!attributes) {
        target->setAttribute(std::make_unique<KeyValueAttribute>());
        attributes = target->getAttribute<KeyValueAttribute>();
      }
      // Preserve optimization contracts such as noinline, but do not expose a
      // specialized entry point to callers that cannot guarantee its type facts.
      util::setExport(target, false);
      attributes->attributes[SPECIALIZED_ATTR] = "";
    }
    return result;
  }

  // Retain exact polymorphic references, including those nested in value records
  // such as tuples, Super wrappers, and partials. Do not follow mutable ref fields.
  void collect(Context &caller, Value *value, Type *type, Path path, Facts &facts,
               size_t &remaining) {
    if (!remaining || path.size() >= MAX_SUMMARY_DEPTH)
      return;
    --remaining;
    if (auto *record = cast<RecordType>(type)) {
      int index = 0;
      for (const auto &field : *record) {
        path.push_back(index++);
        collect(caller, value, field.getType(), path, facts, remaining);
        path.pop_back();
        if (!remaining)
          break;
      }
    } else if (auto *exact = get(caller, value, Path(path.begin() + 1, path.end()))) {
      if (exact->isPolymorphic())
        facts.emplace(std::move(path), exact);
    }
  }

  Context *calleeContext(Context &caller, CallInstr *call, BodiedFunc *callee) {
    Facts facts;
    size_t remaining = MAX_ARGUMENT_FIELDS;
    int index = 0;
    for (auto *argument : *call)
      collect(caller, argument, argument->getType(), {index++}, facts, remaining);
    return context(callee, std::move(facts));
  }

  // A return projection is exact only when every collected return agrees. This
  // also lets callers recover reference facts from functions returning records.
  RefType *returned(Context &context, const Path &fields) {
    auto cached = context.returns.find(fields);
    if (cached != context.returns.end())
      return cached->second;
    if (summaryDepth >= MAX_SUMMARY_DEPTH ||
        !context.activeReturns.insert(fields).second)
      return nullptr;
    ++summaryDepth;
    RefType *result = nullptr;
    for (auto *value : body(context.source).returns) {
      auto *type = get(context, value, fields);
      if (!type || (result && result != type)) {
        result = nullptr;
        break;
      }
      result = type;
    }
    --summaryDepth;
    context.activeReturns.erase(fields);
    context.returns[fields] = result;
    return result;
  }

  RefType *infer(Context &context, const Value *value, Path fields) {
    if (auto *variable = cast<VarValue>(value)) {
      auto &analysis = *reaching.results.at(context.source->getId());
      if (variable->getVar()->isGlobal() || analysis.isInvalid(variable->getVar()))
        return nullptr;
      // Intersect all reaching definitions, including loop-carried assignments.
      // A single unknown or conflicting definition invalidates the proof.
      RefType *result = nullptr;
      for (const auto &definition :
           analysis.getReachingDefinitions(variable->getVar(), value)) {
        RefType *type = nullptr;
        auto *entry =
            cast<analyze::dataflow::SyntheticAssignInstr>(definition.assignment);
        if (entry && entry->getSource() == context.source) {
          // Only CFG entry assignments represent incoming argument facts. Later
          // writes to the same parameter must be analyzed like ordinary locals.
          auto position = std::find(context.source->arg_begin(),
                                    context.source->arg_end(), variable->getVar());
          auto path = fields;
          path.insert(path.begin(),
                      std::distance(context.source->arg_begin(), position));
          auto found = context.arguments.find(path);
          if (found != context.arguments.end())
            type = found->second;
        } else if (definition.known()) {
          type = get(context, definition.assignee, fields);
        }
        if (!type || (result && result != type))
          return nullptr;
        result = type;
      }
      return result;
    }
    if (auto *flow = cast<FlowInstr>(value))
      return get(context, flow->getValue(), fields);
    if (auto *extract = cast<ExtractInstr>(value)) {
      if (auto *record = cast<RecordType>(extract->getVal()->getType())) {
        // Move the projection back onto the containing record, preserving order.
        fields.insert(fields.begin(), record->getMemberIndex(extract->getField()));
        return get(context, extract->getVal(), fields);
      }
    }
    if (auto *conditional = cast<TernaryInstr>(value)) {
      auto *first = get(context, conditional->getTrueValue(), fields);
      return first && first == get(context, conditional->getFalseValue(), fields)
                 ? first
                 : nullptr;
    }
    if (auto *call = cast<CallInstr>(value)) {
      auto *callee = directCallee(call->getCallee());
      if (auto *intrinsic = cast<InternalFunc>(callee)) {
        if (!intrinsic->hasValidIntrinsicSignature())
          return nullptr;
        using Intrinsic = InternalFunc::Intrinsic;
        // Allocation establishes exactness; casts preserve it; record construction
        // routes a projection to its corresponding operand. LLVM bodies are opaque.
        switch (intrinsic->getIntrinsic()) {
        case Intrinsic::ALLOCATE:
          return fields.empty() ? cast<RefType>(intrinsic->getIntrinsicType())
                                : nullptr;
        case Intrinsic::REPRESENTATION_CAST:
          return get(context, call->front(), fields);
        case Intrinsic::RECORD_NEW:
          if (!fields.empty()) {
            auto index = fields.front();
            if (index < 0 || index >= call->numArgs())
              return nullptr;
            fields.erase(fields.begin());
            return get(context, *std::next(call->begin(), index), fields);
          }
          break;
        default:
          break;
        }
      } else if (auto *function = cast<BodiedFunc>(callee)) {
        if (auto *target =
                calleeContext(context, const_cast<CallInstr *>(call), function))
          return returned(*target, fields);
      }
    }
    return nullptr;
  }

  // Memoize proofs within a context. Null means unknown, including cycles and
  // depth-limit failures; it never means the declared type is exact.
  RefType *get(Context &context, const Value *value, const Path &fields = {}) {
    if (!value || body(context.source).hasSharedValues)
      return nullptr;
    Query query{value->getId(), fields};
    auto found = context.cache.find(query);
    if (found != context.cache.end())
      return found->second;
    if (context.active.size() >= MAX_TYPE_DEPTH || fields.size() >= MAX_TYPE_DEPTH ||
        !context.active.insert(query).second)
      return nullptr;
    auto *result = infer(context, value, fields);
    context.active.erase(query);
    context.cache[query] = result;
    return result;
  }

  Func *virtualTarget(Context &context, const Value *value) {
    while (auto *flow = cast<FlowInstr>(value))
      value = flow->getValue();
    auto *call = cast<CallInstr>(value);
    auto *function =
        call ? cast<InternalFunc>(directCallee(call->getCallee())) : nullptr;
    // Exact object layout is stable in the JIT, but method tables can be refreshed
    // by later cells. Only AOT compilation may bind a virtual slot to its target.
    if (jit || !function ||
        function->getIntrinsic() != InternalFunc::Intrinsic::VIRTUAL_LOOKUP ||
        call->numArgs() != 2)
      return nullptr;
    auto *type = get(context, call->front());
    auto *method = cast<IntConst>(call->back());
    auto *info = type ? type->getRuntimeInfo() : nullptr;
    if (!info || !method)
      return nullptr;
    auto target = info->methods.find(method->getVal());
    return target != info->methods.end() && target->second->getType() == call->getType()
               ? target->second
               : nullptr;
  }

  void plan(Context &context) {
    auto &info = body(context.source);
    // Resolve clone identities now: replaceAll changes a source node's getId(),
    // so consulting CloneVisitor after source rewrites could miss the cached clone.
    auto mapped = [&](Value *value) {
      return context.clone ? context.clone->clone(value) : value;
    };
    for (const auto &field : info.fields) {
      RefType *exact = nullptr;
      auto *declared = cast<RefType>(field.second->getType());
      if (declared && declared->isPolymorphic()) {
        auto *type = get(context, field.second);
        auto *runtime = type ? type->getRuntimeInfo() : nullptr;
        // The proof must be compatible with the declared receiver. LLVM uses the
        // original field owner's segment in the exact layout, even with shadowing.
        if (runtime && std::find(runtime->mro.begin(), runtime->mro.end(), declared) !=
                           runtime->mro.end())
          exact = type;
      }
      context.fields.emplace_back(mapped(field.first), exact);
    }
    for (auto *call : info.calls) {
      auto *destination = cast<CallInstr>(mapped(call));
      if (auto *target = virtualTarget(context, call))
        context.lookups.emplace_back(destination, target);
      auto *callee = directCallee(call->getCallee());
      // A proven virtual lookup can also expose a body worth specializing, without
      // waiting for its replacement to invalidate the source analysis.
      if (!callee)
        callee = virtualTarget(context, call->getCallee());
      if (auto *function = cast<BodiedFunc>(callee)) {
        if (auto *target = calleeContext(context, call, function)) {
          if (target->target != function)
            context.calls.emplace_back(destination, target->target);
        }
      }
    }
  }

  void apply(Context &context) {
    for (const auto &field : context.fields) {
      auto *value = field.first;
      value->eraseAttribute(ExactTypeAttribute::AttributeID);
      if (field.second)
        value->setAttribute(std::make_unique<ExactTypeAttribute>(field.second));
    }
    // Preserve callee setup before the existing arguments. Replacing a lookup
    // result must likewise evaluate its callee and arguments once, in order.
    for (const auto &rewrite : context.calls) {
      auto *call = rewrite.first;
      auto *prefix = module->N<SeriesFlow>(call);
      prefix->push_back(call->getCallee());
      call->setCallee(module->N<FlowInstr>(call, prefix,
                                           module->N<VarValue>(call, rewrite.second)));
    }
    for (const auto &rewrite : context.lookups) {
      auto *call = rewrite.first;
      auto *prefix = module->N<SeriesFlow>(call);
      prefix->push_back(call->getCallee());
      for (auto *argument : *call)
        prefix->push_back(argument);
      call->replaceAll(module->N<FlowInstr>(call, prefix,
                                            module->N<VarValue>(call, rewrite.second)));
    }
  }

public:
  Specializer(Module *module, analyze::dataflow::RDResult &reaching, bool jit)
      : module(module), reaching(reaching), jit(jit) {}

  void run() {
    std::vector<BodiedFunc *> functions;
    if (auto *main = cast<BodiedFunc>(module->getMainFunc()))
      functions.push_back(main);
    for (auto *variable : *module) {
      if (auto *function = cast<BodiedFunc>(variable))
        functions.push_back(function);
    }
    for (auto *function : functions)
      context(function);
    // Planning can append newly discovered contexts, so consume a growing worklist.
    // No original body is changed until every context has finished using CFG/RD.
    for (size_t index = 0; index < pending.size(); ++index)
      plan(*pending[index]);
    for (auto *entry : pending)
      apply(*entry);
  }
};

} // namespace

const std::string DevirtualizationPass::KEY = "core-devirtualize";

void DevirtualizationPass::run(Module *module) {
  auto *result = getAnalysisResult<analyze::dataflow::RDResult>(reachingKey);
  Specializer(module, *result, getOptions()->jit).run();
}

} // namespace cleanup
} // namespace transform
} // namespace ir
} // namespace codon
