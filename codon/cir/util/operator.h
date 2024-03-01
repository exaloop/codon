// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <unordered_set>

#include "codon/cir/cir.h"
#include "codon/cir/util/visitor.h"

#define LAMBDA_VISIT(x)                                                                \
  virtual void handle(codon::ir::x *v) {}                                              \
  void visit(codon::ir::x *v) override {                                               \
    if (childrenFirst)                                                                 \
      processChildren(v);                                                              \
    preHook(v);                                                                        \
    handle(v);                                                                         \
    postHook(v);                                                                       \
    if (!childrenFirst)                                                                \
      processChildren(v);                                                              \
  }

namespace codon {
namespace ir {
namespace util {

/// Pass that visits all values in a module.
class Operator : public Visitor {
private:
  /// IDs of previously visited nodes
  std::unordered_set<id_t> seen;
  /// stack of IR nodes being visited
  std::vector<Node *> nodeStack;
  /// stack of iterators
  std::vector<decltype(SeriesFlow().begin())> itStack;
  /// true if should visit children first
  bool childrenFirst;

protected:
  void defaultVisit(Node *) override {}

public:
  /// Constructs an operator.
  /// @param childrenFirst true if children should be visited first
  explicit Operator(bool childrenFirst = false) : childrenFirst(childrenFirst) {}

  virtual ~Operator() noexcept = default;

  /// This function is applied to all nodes before handling the node
  /// itself. It provides a way to write one function that gets
  /// applied to every visited node.
  /// @param node the node
  virtual void preHook(Node *node) {}
  /// This function is applied to all nodes after handling the node
  /// itself. It provides a way to write one function that gets
  /// applied to every visited node.
  /// @param node the node
  virtual void postHook(Node *node) {}

  void visit(Module *m) override {
    nodeStack.push_back(m);
    nodeStack.push_back(m->getMainFunc());
    process(m->getMainFunc());
    nodeStack.pop_back();
    for (auto *s : *m) {
      nodeStack.push_back(s);
      process(s);
      nodeStack.pop_back();
    }
    nodeStack.pop_back();
  }

  void visit(BodiedFunc *f) override {
    if (f->getBody()) {
      seen.insert(f->getBody()->getId());
      process(f->getBody());
    }
  }

  LAMBDA_VISIT(VarValue);
  LAMBDA_VISIT(PointerValue);

  void visit(codon::ir::SeriesFlow *v) override {
    if (childrenFirst)
      processSeriesFlowChildren(v);
    preHook(v);
    handle(v);
    postHook(v);
    if (!childrenFirst)
      processSeriesFlowChildren(v);
  }

  virtual void handle(codon::ir::SeriesFlow *v) {}
  LAMBDA_VISIT(IfFlow);
  LAMBDA_VISIT(WhileFlow);
  LAMBDA_VISIT(ForFlow);
  LAMBDA_VISIT(ImperativeForFlow);
  LAMBDA_VISIT(TryCatchFlow);
  LAMBDA_VISIT(PipelineFlow);
  LAMBDA_VISIT(dsl::CustomFlow);

  LAMBDA_VISIT(TemplatedConst<int64_t>);
  LAMBDA_VISIT(TemplatedConst<double>);
  LAMBDA_VISIT(TemplatedConst<bool>);
  LAMBDA_VISIT(TemplatedConst<std::string>);
  LAMBDA_VISIT(dsl::CustomConst);

  LAMBDA_VISIT(Instr);
  LAMBDA_VISIT(AssignInstr);
  LAMBDA_VISIT(ExtractInstr);
  LAMBDA_VISIT(InsertInstr);
  LAMBDA_VISIT(CallInstr);
  LAMBDA_VISIT(StackAllocInstr);
  LAMBDA_VISIT(TypePropertyInstr);
  LAMBDA_VISIT(YieldInInstr);
  LAMBDA_VISIT(TernaryInstr);
  LAMBDA_VISIT(BreakInstr);
  LAMBDA_VISIT(ContinueInstr);
  LAMBDA_VISIT(ReturnInstr);
  LAMBDA_VISIT(YieldInstr);
  LAMBDA_VISIT(ThrowInstr);
  LAMBDA_VISIT(FlowInstr);
  LAMBDA_VISIT(dsl::CustomInstr);

  template <typename Node> void process(Node *v) { v->accept(*this); }

  /// Return the parent of the current node.
  /// @param level the number of levels up from the current node
  template <typename Desired = Node> Desired *getParent(int level = 0) {
    return cast<Desired>(nodeStack[nodeStack.size() - level - 1]);
  }
  /// @return current depth in the tree
  int depth() const { return nodeStack.size(); }

  /// @tparam Desired the desired type
  /// @return the last encountered example of the desired type
  template <typename Desired> Desired *findLast() {
    for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); ++it) {
      if (auto *v = cast<Desired>(*it))
        return v;
    }
    return nullptr;
  }
  /// @return the last encountered function
  Func *getParentFunc() { return findLast<Func>(); }

  /// @return an iterator to the first parent
  auto parent_begin() const { return nodeStack.begin(); }
  /// @return an iterator beyond the last parent
  auto parent_end() const { return nodeStack.end(); }

  /// @param v the value
  /// @return whether we have visited ("seen") the given value
  bool saw(const Value *v) const { return seen.find(v->getId()) != seen.end(); }
  /// Avoid visiting the given value in the future.
  /// @param v the value
  void see(const Value *v) { seen.insert(v->getId()); }

  /// Inserts the new value before the current position in the last seen SeriesFlow.
  /// @param v the new value
  auto insertBefore(Value *v) {
    return findLast<SeriesFlow>()->insert(itStack.back(), v);
  }
  /// Inserts the new value after the current position in the last seen SeriesFlow.
  /// @param v the new value, which is marked seen
  auto insertAfter(Value *v) {
    auto newPos = itStack.back();
    ++newPos;
    see(v);

    return findLast<SeriesFlow>()->insert(newPos, v);
  }

  /// Resets the operator.
  void reset() {
    seen.clear();
    nodeStack.clear();
    itStack.clear();
  }

private:
  void processChildren(Value *v) {
    nodeStack.push_back(v);
    for (auto *c : v->getUsedValues()) {
      if (saw(c))
        continue;
      see(c);
      process(c);
    }
    nodeStack.pop_back();
  }

  void processSeriesFlowChildren(codon::ir::SeriesFlow *v) {
    nodeStack.push_back(v);
    for (auto it = v->begin(); it != v->end(); ++it) {
      itStack.push_back(it);
      process(*it);
      itStack.pop_back();
    }
    nodeStack.pop_back();
  }
};

} // namespace util
} // namespace ir
} // namespace codon

#undef LAMBDA_VISIT
