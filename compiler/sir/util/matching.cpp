#include "matching.h"

#include <algorithm>

#include "sir/sir.h"

#include "visitor.h"

#define VISIT(x)                                                                       \
  void visit(const x *v) override {                                                    \
    if (matchAny || dynamic_cast<const util::Any *>(v)) {                              \
      result = true;                                                                   \
      matchAny = true;                                                                 \
    } else if (!nodeId) {                                                              \
      nodeId = &x::NodeId;                                                             \
      other = v;                                                                       \
    } else if (nodeId != &x::NodeId ||                                                 \
               (!checkName && v->getName() != other->getName()))                       \
      result = false;                                                                  \
    else                                                                               \
      handle(v, static_cast<const x *>(other));                                        \
  }

namespace seq {
namespace ir {
namespace util {
namespace {
class MatchVisitor : public util::ConstVisitor {
private:
  bool matchAny = false;
  bool checkName;
  const char *nodeId = nullptr;
  bool result = false;
  const Node *other = nullptr;
  bool varIdMatch;

public:
  explicit MatchVisitor(bool checkName = false, bool varIdMatch = false)
      : checkName(checkName), varIdMatch(varIdMatch) {}

  VISIT(Var);
  void handle(const Var *x, const Var *y) { result = compareVars(x, y); }

  VISIT(Func);
  void handle(const Func *x, const Func *y) {}
  VISIT(BodiedFunc);
  void handle(const BodiedFunc *x, const BodiedFunc *y) {
    result = compareFuncs(x, y) &&
             std::equal(x->begin(), x->end(), y->begin(), y->end(),
                        [this](auto *x, auto *y) { return process(x, y); }) &&
             process(x->getBody(), y->getBody()) && x->isBuiltin() == y->isBuiltin();
  }
  VISIT(ExternalFunc);
  void handle(const ExternalFunc *x, const ExternalFunc *y) {
    result = x->getUnmangledName() == y->getUnmangledName() && compareFuncs(x, y);
  }
  VISIT(InternalFunc);
  void handle(const InternalFunc *x, const InternalFunc *y) {
    result = x->getParentType() == y->getParentType() && compareFuncs(x, y);
  }
  VISIT(LLVMFunc);
  void handle(const LLVMFunc *x, const LLVMFunc *y) {
    result = std::equal(x->literal_begin(), x->literal_end(), y->literal_begin(),
                        y->literal_end(),
                        [this](auto &x, auto &y) {
                          if (x.isStatic() && y.isStatic())
                            return x.getStaticValue() == y.getStaticValue();
                          else if (x.isType() && y.isType())
                            return process(x.getTypeValue(), y.getTypeValue());
                          return false;
                        }) &&
             x->getLLVMDeclarations() == y->getLLVMDeclarations() &&
             x->getLLVMBody() == y->getLLVMBody() && compareFuncs(x, y);
  }

  VISIT(Value);
  void handle(const Value *x, const Value *y) {}
  VISIT(VarValue);
  void handle(const VarValue *x, const VarValue *y) {
    result = compareVars(x->getVar(), y->getVar());
  }
  VISIT(PointerValue);
  void handle(const PointerValue *x, const PointerValue *y) {
    result = compareVars(x->getVar(), y->getVar());
  }

  VISIT(Flow);
  void handle(const Flow *x, const Flow *y) {}
  VISIT(SeriesFlow);
  void handle(const SeriesFlow *x, const SeriesFlow *y) {
    result = std::equal(x->begin(), x->end(), y->begin(), y->end(),
                        [this](auto *x, auto *y) { return process(x, y); });
  }
  VISIT(IfFlow);
  void handle(const IfFlow *x, const IfFlow *y) {
    result = process(x->getCond(), y->getCond()) &&
             process(x->getTrueBranch(), y->getTrueBranch()) &&
             process(x->getFalseBranch(), y->getFalseBranch());
  }

  VISIT(WhileFlow);
  void handle(const WhileFlow *x, const WhileFlow *y) {
    result = process(x->getCond(), y->getCond()) && process(x->getBody(), y->getBody());
  }
  VISIT(ForFlow);
  void handle(const ForFlow *x, const ForFlow *y) {
    result = process(x->getIter(), y->getIter()) &&
             process(x->getBody(), y->getBody()) && process(x->getVar(), y->getVar());
  }
  VISIT(ImperativeForFlow);
  void handle(const ImperativeForFlow *x, const ImperativeForFlow *y) {
    result = process(x->getVar(), y->getVar()) && process(x->getBody(), y->getBody()) &&
             process(x->getStart(), y->getStart()) && x->getStep() == y->getStep() &&
             process(x->getEnd(), y->getEnd());
  }
  VISIT(TryCatchFlow);
  void handle(const TryCatchFlow *x, const TryCatchFlow *y) {
    result = result && process(x->getFinally(), y->getFinally()) &&
             process(x->getBody(), y->getBody()) &&
             std::equal(x->begin(), x->end(), y->begin(), y->end(),
                        [this](auto &x, auto &y) {
                          return process(x.getHandler(), y.getHandler()) &&
                                 process(x.getType(), y.getType()) &&
                                 process(x.getVar(), y.getVar());
                        });
  }
  VISIT(PipelineFlow);
  void handle(const PipelineFlow *x, const PipelineFlow *y) {
    result = std::equal(
        x->begin(), x->end(), y->begin(), y->end(), [this](auto &x, auto &y) {
          return process(x.getCallee(), y.getCallee()) &&
                 std::equal(x.begin(), x.end(), y.begin(), y.end(),
                            [this](auto *x, auto *y) { return process(x, y); }) &&
                 x.isGenerator() == y.isGenerator() && x.isParallel() == y.isParallel();
        });
  }
  VISIT(dsl::CustomFlow);
  void handle(const dsl::CustomFlow *x, const dsl::CustomFlow *y) {
    result = x->match(y);
  }

  VISIT(IntConst);
  void handle(const IntConst *x, const IntConst *y) {
    result = process(x->getType(), y->getType()) && x->getVal() == y->getVal();
  }
  VISIT(FloatConst);
  void handle(const FloatConst *x, const FloatConst *y) {
    result = process(x->getType(), y->getType()) && x->getVal() == y->getVal();
  }
  VISIT(BoolConst);
  void handle(const BoolConst *x, const BoolConst *y) {
    result = process(x->getType(), y->getType()) && x->getVal() == y->getVal();
  }
  VISIT(StringConst);
  void handle(const StringConst *x, const StringConst *y) {
    result = process(x->getType(), y->getType()) && x->getVal() == y->getVal();
  }
  VISIT(dsl::CustomConst);
  void handle(const dsl::CustomConst *x, const dsl::CustomConst *y) {
    result = x->match(y);
  }

  VISIT(AssignInstr);
  void handle(const AssignInstr *x, const AssignInstr *y) {
    result = process(x->getLhs(), y->getLhs()) && process(x->getRhs(), y->getRhs());
  }
  VISIT(ExtractInstr);
  void handle(const ExtractInstr *x, const ExtractInstr *y) {
    result = process(x->getVal(), y->getVal()) && x->getField() == y->getField();
  }
  VISIT(InsertInstr);
  void handle(const InsertInstr *x, const InsertInstr *y) {
    result = process(x->getLhs(), y->getLhs()) && x->getField() == y->getField() &&
             process(x->getRhs(), y->getRhs());
  }
  VISIT(CallInstr);
  void handle(const CallInstr *x, const CallInstr *y) {
    result = process(x->getCallee(), y->getCallee()) &&
             std::equal(x->begin(), x->end(), y->begin(), y->end(),
                        [this](auto *x, auto *y) { return process(x, y); });
  }
  VISIT(StackAllocInstr);
  void handle(const StackAllocInstr *x, const StackAllocInstr *y) {
    result = x->getCount() == y->getCount() && process(x->getType(), y->getType());
  }
  VISIT(TypePropertyInstr);
  void handle(const TypePropertyInstr *x, const TypePropertyInstr *y) {
    result = x->getProperty() == y->getProperty() &&
             process(x->getInspectType(), y->getInspectType());
  }
  VISIT(YieldInInstr);
  void handle(const YieldInInstr *x, const YieldInInstr *y) {
    result = process(x->getType(), y->getType());
  }
  VISIT(TernaryInstr);
  void handle(const TernaryInstr *x, const TernaryInstr *y) {
    result = process(x->getCond(), y->getCond()) &&
             process(x->getTrueValue(), y->getTrueValue()) &&
             process(x->getFalseValue(), y->getFalseValue());
  }
  VISIT(BreakInstr);
  void handle(const BreakInstr *x, const BreakInstr *y) {
    result = process(x->getLoop(), y->getLoop());
  }
  VISIT(ContinueInstr);
  void handle(const ContinueInstr *x, const ContinueInstr *y) {
    result = process(x->getLoop(), y->getLoop());
  }
  VISIT(ReturnInstr);
  void handle(const ReturnInstr *x, const ReturnInstr *y) {
    result = process(x->getValue(), y->getValue());
  }
  VISIT(YieldInstr);
  void handle(const YieldInstr *x, const YieldInstr *y) {
    result = process(x->getValue(), y->getValue());
  }
  VISIT(ThrowInstr);
  void handle(const ThrowInstr *x, const ThrowInstr *y) {
    result = process(x->getValue(), y->getValue());
  }
  VISIT(FlowInstr);
  void handle(const FlowInstr *x, const FlowInstr *y) {
    result =
        process(x->getFlow(), y->getFlow()) && process(x->getValue(), y->getValue());
  }
  VISIT(dsl::CustomInstr);
  void handle(const dsl::CustomInstr *x, const dsl::CustomInstr *y) {
    result = x->match(y);
  }

  VISIT(types::Type);
  void handle(const types::Type *x, const types::Type *y) {}
  VISIT(types::IntType);
  void handle(const types::IntType *, const types::IntType *) { result = true; }
  VISIT(types::FloatType);
  void handle(const types::FloatType *, const types::FloatType *) { result = true; }
  VISIT(types::BoolType);
  void handle(const types::BoolType *, const types::BoolType *) { result = true; }
  VISIT(types::ByteType);
  void handle(const types::ByteType *, const types::ByteType *) { result = true; }
  VISIT(types::VoidType);
  void handle(const types::VoidType *, const types::VoidType *) { result = true; }
  VISIT(types::RecordType);
  void handle(const types::RecordType *x, const types::RecordType *y) {
    result = std::equal(
        x->begin(), x->end(), y->begin(), y->end(), [this](auto &x, auto &y) {
          return x.getName() == y.getName() && process(x.getType(), y.getType());
        });
  }
  VISIT(types::RefType);
  void handle(const types::RefType *x, const types::RefType *y) {
    result = process(x->getContents(), y->getContents());
  }
  VISIT(types::FuncType);
  void handle(const types::FuncType *x, const types::FuncType *y) {
    result = process(x->getReturnType(), y->getReturnType()) &&
             std::equal(x->begin(), x->end(), y->begin(), y->end(),
                        [this](auto *x, auto *y) { return process(x, y); });
  }
  VISIT(types::OptionalType);
  void handle(const types::OptionalType *x, const types::OptionalType *y) {
    result = process(x->getBase(), y->getBase());
  }
  VISIT(types::PointerType);
  void handle(const types::PointerType *x, const types::PointerType *y) {
    result = process(x->getBase(), y->getBase());
  }
  VISIT(types::GeneratorType);
  void handle(const types::GeneratorType *x, const types::GeneratorType *y) {
    result = process(x->getBase(), y->getBase());
  }
  VISIT(types::IntNType);
  void handle(const types::IntNType *x, const types::IntNType *y) {
    result = x->getLen() == y->getLen() && x->isSigned() == y->isSigned();
  }
  VISIT(dsl::types::CustomType);
  void handle(const dsl::types::CustomType *x, const dsl::types::CustomType *y) {
    result = x->match(y);
  }

  bool process(const Node *x, const Node *y) const {
    if (!x && !y)
      return true;
    else if ((!x && y) || (x && !y))
      return false;

    MatchVisitor v(checkName);
    x->accept(v);
    y->accept(v);

    return v.result;
  }

private:
  bool compareVars(const Var *x, const Var *y) const {
    return process(x->getType(), y->getType()) &&
           (!varIdMatch || x->getId() == y->getId());
  }

  bool compareFuncs(const Func *x, const Func *y) const {
    if (!compareVars(x, y))
      return false;

    if (!std::equal(x->arg_begin(), x->arg_end(), y->arg_begin(), y->arg_end(),
                    [this](auto *x, auto *y) { return process(x, y); }))
      return false;

    return true;
  }
};
} // namespace

const char AnyType::NodeId = 0;

const char AnyValue::NodeId = 0;

const char AnyFlow::NodeId = 0;

const char AnyVar::NodeId = 0;

const char AnyFunc::NodeId = 0;

bool match(Node *a, Node *b, bool checkNames, bool varIdMatch) {
  return MatchVisitor(checkNames).process(a, b);
}

} // namespace util
} // namespace ir
} // namespace seq

#undef VISIT
