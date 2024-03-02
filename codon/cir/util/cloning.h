// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <unordered_map>

#include "codon/cir/cir.h"
#include "codon/cir/util/visitor.h"

namespace codon {
namespace ir {
namespace util {

class CloneVisitor : public ConstVisitor {
private:
  /// the clone context
  std::unordered_map<int, Node *> ctx;
  /// the result
  Node *result;
  /// the module
  Module *module;
  /// true if break/continue loops should be cloned
  bool cloneLoop;

public:
  /// Constructs a clone visitor.
  /// @param module the module
  /// @param cloneLoop true if break/continue loops should be cloned
  explicit CloneVisitor(Module *module, bool cloneLoop = true)
      : ctx(), result(nullptr), module(module), cloneLoop(cloneLoop) {}

  virtual ~CloneVisitor() noexcept = default;

  void visit(const Var *v) override;

  void visit(const BodiedFunc *v) override;
  void visit(const ExternalFunc *v) override;
  void visit(const InternalFunc *v) override;
  void visit(const LLVMFunc *v) override;

  void visit(const VarValue *v) override;
  void visit(const PointerValue *v) override;

  void visit(const SeriesFlow *v) override;
  void visit(const IfFlow *v) override;
  void visit(const WhileFlow *v) override;
  void visit(const ForFlow *v) override;
  void visit(const ImperativeForFlow *v) override;
  void visit(const TryCatchFlow *v) override;
  void visit(const PipelineFlow *v) override;
  void visit(const dsl::CustomFlow *v) override;

  void visit(const IntConst *v) override;
  void visit(const FloatConst *v) override;
  void visit(const BoolConst *v) override;
  void visit(const StringConst *v) override;
  void visit(const dsl::CustomConst *v) override;

  void visit(const AssignInstr *v) override;
  void visit(const ExtractInstr *v) override;
  void visit(const InsertInstr *v) override;
  void visit(const CallInstr *v) override;
  void visit(const StackAllocInstr *v) override;
  void visit(const TypePropertyInstr *v) override;
  void visit(const YieldInInstr *v) override;
  void visit(const TernaryInstr *v) override;
  void visit(const BreakInstr *v) override;
  void visit(const ContinueInstr *v) override;
  void visit(const ReturnInstr *v) override;
  void visit(const YieldInstr *v) override;
  void visit(const ThrowInstr *v) override;
  void visit(const FlowInstr *v) override;
  void visit(const dsl::CustomInstr *v) override;

  /// Clones a value, returning the previous value if other has already been cloned.
  /// @param other the original
  /// @param cloneTo the function to clone locals to, or null if none
  /// @param remaps variable re-mappings
  /// @return the clone
  Value *clone(const Value *other, BodiedFunc *cloneTo = nullptr,
               const std::unordered_map<id_t, Var *> &remaps = {});

  /// Returns the original unless the variable has been force cloned.
  /// @param other the original
  /// @return the original or the previous clone
  Var *clone(const Var *other);

  /// Clones a flow, returning the previous value if other has already been cloned.
  /// @param other the original
  /// @return the clone
  Flow *clone(const Flow *other) {
    return cast<Flow>(clone(static_cast<const Value *>(other)));
  }

  /// Forces a clone. No difference for values but ensures that variables are actually
  /// cloned.
  /// @param other the original
  /// @return the clone
  template <typename NodeType> NodeType *forceClone(const NodeType *other) {
    if (!other)
      return nullptr;

    auto id = other->getId();
    if (ctx.find(id) == ctx.end()) {
      other->accept(*this);
      ctx[id] = result;

      for (auto it = other->attributes_begin(); it != other->attributes_end(); ++it) {
        const auto *attr = other->getAttribute(*it);
        if (attr->needsClone()) {
          ctx[id]->setAttribute(attr->forceClone(*this), *it);
        }
      }
    }
    return cast<NodeType>(ctx[id]);
  }

  /// Remaps a clone.
  /// @param original the original
  /// @param newVal the clone
  template <typename NodeType>
  void forceRemap(const NodeType *original, const NodeType *newVal) {
    ctx[original->getId()] = const_cast<NodeType *>(newVal);
  }

  PipelineFlow::Stage clone(const PipelineFlow::Stage &other) {
    std::vector<Value *> args;
    for (const auto *a : other)
      args.push_back(clone(a));
    return {clone(other.getCallee()), std::move(args), other.isGenerator(),
            other.isParallel()};
  }

private:
  template <typename NodeType, typename... Args>
  NodeType *Nt(const NodeType *source, Args... args) {
    return module->N<NodeType>(source, std::forward<Args>(args)..., source->getName());
  }
};

} // namespace util
} // namespace ir
} // namespace codon
