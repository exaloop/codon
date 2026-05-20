// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#define VISIT(x) virtual void visit(codon::ir::x *)
#define CONST_VISIT(x) virtual void visit(const codon::ir::x *)

namespace codon {
namespace ir {
class Node;

class Type;
class PrimitiveType;
class IntType;
class FloatType;
class Float32Type;
class Float16Type;
class BFloat16Type;
class Float128Type;
class BoolType;
class RecordType;
class RefType;
class FuncType;
class OptionalType;
class PointerType;
class GeneratorType;
class VectorType;
class UnionType;

namespace dsl {
class CustomType;
class CustomConst;
class CustomFlow;
class CustomInstr;
} // namespace dsl

class Module;

class Var;

class Func;
class BodiedFunc;
class ExternalFunc;
class InternalFunc;
class LLVMFunc;

class Value;
class VarValue;
class PointerValue;

class Flow;
class SeriesFlow;
class IfFlow;
class WhileFlow;
class ForFlow;
class ImperativeForFlow;
class TryCatchFlow;
class PipelineFlow;

class Const;

template <typename ValueType> class TemplatedConst;

class Instr;
class AssignInstr;
class ExtractInstr;
class InsertInstr;
class CallInstr;
class StackAllocInstr;
class TypePropertyInstr;
class YieldInInstr;
class TernaryInstr;
class BreakInstr;
class ContinueInstr;
class ReturnInstr;
class YieldInstr;
class AwaitInstr;
class ThrowInstr;
class FlowInstr;

namespace util {

/// Base for CIR visitors
class Visitor {
protected:
  virtual void defaultVisit(codon::ir::Node *) {
    throw std::runtime_error("cannot visit node");
  }

public:
  virtual ~Visitor() noexcept = default;

  VISIT(Module);

  VISIT(Var);

  VISIT(Func);
  VISIT(BodiedFunc);
  VISIT(ExternalFunc);
  VISIT(InternalFunc);
  VISIT(LLVMFunc);

  VISIT(Value);
  VISIT(VarValue);
  VISIT(PointerValue);

  VISIT(Flow);
  VISIT(SeriesFlow);
  VISIT(IfFlow);
  VISIT(WhileFlow);
  VISIT(ForFlow);
  VISIT(ImperativeForFlow);
  VISIT(TryCatchFlow);
  VISIT(PipelineFlow);
  VISIT(dsl::CustomFlow);

  VISIT(Const);
  VISIT(TemplatedConst<int64_t>);
  VISIT(TemplatedConst<double>);
  VISIT(TemplatedConst<bool>);
  VISIT(TemplatedConst<std::string>);
  VISIT(dsl::CustomConst);

  VISIT(Instr);
  VISIT(AssignInstr);
  VISIT(ExtractInstr);
  VISIT(InsertInstr);
  VISIT(CallInstr);
  VISIT(StackAllocInstr);
  VISIT(TypePropertyInstr);
  VISIT(YieldInInstr);
  VISIT(TernaryInstr);
  VISIT(BreakInstr);
  VISIT(ContinueInstr);
  VISIT(ReturnInstr);
  VISIT(YieldInstr);
  VISIT(AwaitInstr);
  VISIT(ThrowInstr);
  VISIT(FlowInstr);
  VISIT(dsl::CustomInstr);

  VISIT(Type);
  VISIT(PrimitiveType);
  VISIT(IntType);
  VISIT(FloatType);
  VISIT(Float32Type);
  VISIT(Float16Type);
  VISIT(BFloat16Type);
  VISIT(Float128Type);
  VISIT(BoolType);
  VISIT(RecordType);
  VISIT(RefType);
  VISIT(FuncType);
  VISIT(OptionalType);
  VISIT(PointerType);
  VISIT(GeneratorType);
  VISIT(VectorType);
  VISIT(UnionType);
  VISIT(dsl::CustomType);
};

class ConstVisitor {
protected:
  virtual void defaultVisit(const codon::ir::Node *) {
    throw std::runtime_error("cannot visit const node");
  }

public:
  virtual ~ConstVisitor() noexcept = default;

  CONST_VISIT(Module);

  CONST_VISIT(Var);

  CONST_VISIT(Func);
  CONST_VISIT(BodiedFunc);
  CONST_VISIT(ExternalFunc);
  CONST_VISIT(InternalFunc);
  CONST_VISIT(LLVMFunc);

  CONST_VISIT(Value);
  CONST_VISIT(VarValue);
  CONST_VISIT(PointerValue);

  CONST_VISIT(Flow);
  CONST_VISIT(SeriesFlow);
  CONST_VISIT(IfFlow);
  CONST_VISIT(WhileFlow);
  CONST_VISIT(ForFlow);
  CONST_VISIT(ImperativeForFlow);
  CONST_VISIT(TryCatchFlow);
  CONST_VISIT(PipelineFlow);
  CONST_VISIT(dsl::CustomFlow);

  CONST_VISIT(Const);
  CONST_VISIT(TemplatedConst<int64_t>);
  CONST_VISIT(TemplatedConst<double>);
  CONST_VISIT(TemplatedConst<bool>);
  CONST_VISIT(TemplatedConst<std::string>);
  CONST_VISIT(dsl::CustomConst);

  CONST_VISIT(Instr);
  CONST_VISIT(AssignInstr);
  CONST_VISIT(ExtractInstr);
  CONST_VISIT(InsertInstr);
  CONST_VISIT(CallInstr);
  CONST_VISIT(StackAllocInstr);
  CONST_VISIT(TypePropertyInstr);
  CONST_VISIT(YieldInInstr);
  CONST_VISIT(TernaryInstr);
  CONST_VISIT(BreakInstr);
  CONST_VISIT(ContinueInstr);
  CONST_VISIT(ReturnInstr);
  CONST_VISIT(YieldInstr);
  CONST_VISIT(AwaitInstr);
  CONST_VISIT(ThrowInstr);
  CONST_VISIT(FlowInstr);
  CONST_VISIT(dsl::CustomInstr);

  CONST_VISIT(Type);
  CONST_VISIT(PrimitiveType);
  CONST_VISIT(IntType);
  CONST_VISIT(FloatType);
  CONST_VISIT(Float32Type);
  CONST_VISIT(Float16Type);
  CONST_VISIT(BFloat16Type);
  CONST_VISIT(Float128Type);
  CONST_VISIT(BoolType);
  CONST_VISIT(RecordType);
  CONST_VISIT(RefType);
  CONST_VISIT(FuncType);
  CONST_VISIT(OptionalType);
  CONST_VISIT(PointerType);
  CONST_VISIT(GeneratorType);
  CONST_VISIT(VectorType);
  CONST_VISIT(UnionType);
  CONST_VISIT(dsl::CustomType);
};

} // namespace util
} // namespace ir
} // namespace codon

#undef VISIT
#undef CONST_VISIT
