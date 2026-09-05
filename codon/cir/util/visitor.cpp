// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "visitor.h"

#include "codon/cir/cir.h"

namespace codon {
namespace ir {
namespace util {

void Visitor::visit(Module *x) { defaultVisit(x); }
void Visitor::visit(Var *x) { defaultVisit(x); }
void Visitor::visit(Func *x) { defaultVisit(x); }
void Visitor::visit(BodiedFunc *x) { defaultVisit(x); }
void Visitor::visit(ExternalFunc *x) { defaultVisit(x); }
void Visitor::visit(InternalFunc *x) { defaultVisit(x); }
void Visitor::visit(LLVMFunc *x) { defaultVisit(x); }
void Visitor::visit(Value *x) { defaultVisit(x); }
void Visitor::visit(VarValue *x) { defaultVisit(x); }
void Visitor::visit(PointerValue *x) { defaultVisit(x); }
void Visitor::visit(Flow *x) { defaultVisit(x); }
void Visitor::visit(SeriesFlow *x) { defaultVisit(x); }
void Visitor::visit(IfFlow *x) { defaultVisit(x); }
void Visitor::visit(WhileFlow *x) { defaultVisit(x); }
void Visitor::visit(ForFlow *x) { defaultVisit(x); }
void Visitor::visit(ImperativeForFlow *x) { defaultVisit(x); }
void Visitor::visit(TryCatchFlow *x) { defaultVisit(x); }
void Visitor::visit(PipelineFlow *x) { defaultVisit(x); }
void Visitor::visit(dsl::CustomFlow *x) { defaultVisit(x); }
void Visitor::visit(Const *x) { defaultVisit(x); }
void Visitor::visit(TemplatedConst<int64_t> *x) { defaultVisit(x); }
void Visitor::visit(TemplatedConst<double> *x) { defaultVisit(x); }
void Visitor::visit(TemplatedConst<bool> *x) { defaultVisit(x); }
void Visitor::visit(StringConst *x) { defaultVisit(x); }
void Visitor::visit(BytesConst *x) { defaultVisit(x); }
void Visitor::visit(dsl::CustomConst *x) { defaultVisit(x); }
void Visitor::visit(Instr *x) { defaultVisit(x); }
void Visitor::visit(AssignInstr *x) { defaultVisit(x); }
void Visitor::visit(ExtractInstr *x) { defaultVisit(x); }
void Visitor::visit(InsertInstr *x) { defaultVisit(x); }
void Visitor::visit(CallInstr *x) { defaultVisit(x); }
void Visitor::visit(StackAllocInstr *x) { defaultVisit(x); }
void Visitor::visit(YieldInInstr *x) { defaultVisit(x); }
void Visitor::visit(TernaryInstr *x) { defaultVisit(x); }
void Visitor::visit(BreakInstr *x) { defaultVisit(x); }
void Visitor::visit(ContinueInstr *x) { defaultVisit(x); }
void Visitor::visit(ReturnInstr *x) { defaultVisit(x); }
void Visitor::visit(TypePropertyInstr *x) { defaultVisit(x); }
void Visitor::visit(YieldInstr *x) { defaultVisit(x); }
void Visitor::visit(AwaitInstr *x) { defaultVisit(x); }
void Visitor::visit(ThrowInstr *x) { defaultVisit(x); }
void Visitor::visit(FlowInstr *x) { defaultVisit(x); }
void Visitor::visit(dsl::CustomInstr *x) { defaultVisit(x); }
void Visitor::visit(Type *x) { defaultVisit(x); }
void Visitor::visit(PrimitiveType *x) { defaultVisit(x); }
void Visitor::visit(IntType *x) { defaultVisit(x); }
void Visitor::visit(FloatType *x) { defaultVisit(x); }
void Visitor::visit(Float32Type *x) { defaultVisit(x); }
void Visitor::visit(Float16Type *x) { defaultVisit(x); }
void Visitor::visit(BFloat16Type *x) { defaultVisit(x); }
void Visitor::visit(Float128Type *x) { defaultVisit(x); }
void Visitor::visit(BoolType *x) { defaultVisit(x); }
void Visitor::visit(RecordType *x) { defaultVisit(x); }
void Visitor::visit(RefType *x) { defaultVisit(x); }
void Visitor::visit(FuncType *x) { defaultVisit(x); }
void Visitor::visit(OptionalType *x) { defaultVisit(x); }
void Visitor::visit(PointerType *x) { defaultVisit(x); }
void Visitor::visit(GeneratorType *x) { defaultVisit(x); }
void Visitor::visit(VectorType *x) { defaultVisit(x); }
void Visitor::visit(UnionType *x) { defaultVisit(x); }
void Visitor::visit(dsl::CustomType *x) { defaultVisit(x); }

void ConstVisitor::visit(const Module *x) { defaultVisit(x); }
void ConstVisitor::visit(const Var *x) { defaultVisit(x); }
void ConstVisitor::visit(const Func *x) { defaultVisit(x); }
void ConstVisitor::visit(const BodiedFunc *x) { defaultVisit(x); }
void ConstVisitor::visit(const ExternalFunc *x) { defaultVisit(x); }
void ConstVisitor::visit(const InternalFunc *x) { defaultVisit(x); }
void ConstVisitor::visit(const LLVMFunc *x) { defaultVisit(x); }
void ConstVisitor::visit(const Value *x) { defaultVisit(x); }
void ConstVisitor::visit(const VarValue *x) { defaultVisit(x); }
void ConstVisitor::visit(const PointerValue *x) { defaultVisit(x); }
void ConstVisitor::visit(const Flow *x) { defaultVisit(x); }
void ConstVisitor::visit(const SeriesFlow *x) { defaultVisit(x); }
void ConstVisitor::visit(const IfFlow *x) { defaultVisit(x); }
void ConstVisitor::visit(const WhileFlow *x) { defaultVisit(x); }
void ConstVisitor::visit(const ForFlow *x) { defaultVisit(x); }
void ConstVisitor::visit(const ImperativeForFlow *x) { defaultVisit(x); }
void ConstVisitor::visit(const TryCatchFlow *x) { defaultVisit(x); }
void ConstVisitor::visit(const PipelineFlow *x) { defaultVisit(x); }
void ConstVisitor::visit(const dsl::CustomFlow *x) { defaultVisit(x); }
void ConstVisitor::visit(const Const *x) { defaultVisit(x); }
void ConstVisitor::visit(const TemplatedConst<int64_t> *x) { defaultVisit(x); }
void ConstVisitor::visit(const TemplatedConst<double> *x) { defaultVisit(x); }
void ConstVisitor::visit(const TemplatedConst<bool> *x) { defaultVisit(x); }
void ConstVisitor::visit(const StringConst *x) { defaultVisit(x); }
void ConstVisitor::visit(const BytesConst *x) { defaultVisit(x); }
void ConstVisitor::visit(const dsl::CustomConst *x) { defaultVisit(x); }
void ConstVisitor::visit(const Instr *x) { defaultVisit(x); }
void ConstVisitor::visit(const AssignInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const ExtractInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const InsertInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const CallInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const StackAllocInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const YieldInInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const TernaryInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const BreakInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const ContinueInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const ReturnInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const TypePropertyInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const YieldInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const AwaitInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const ThrowInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const FlowInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const dsl::CustomInstr *x) { defaultVisit(x); }
void ConstVisitor::visit(const Type *x) { defaultVisit(x); }
void ConstVisitor::visit(const PrimitiveType *x) { defaultVisit(x); }
void ConstVisitor::visit(const IntType *x) { defaultVisit(x); }
void ConstVisitor::visit(const FloatType *x) { defaultVisit(x); }
void ConstVisitor::visit(const Float32Type *x) { defaultVisit(x); }
void ConstVisitor::visit(const Float16Type *x) { defaultVisit(x); }
void ConstVisitor::visit(const BFloat16Type *x) { defaultVisit(x); }
void ConstVisitor::visit(const Float128Type *x) { defaultVisit(x); }
void ConstVisitor::visit(const BoolType *x) { defaultVisit(x); }
void ConstVisitor::visit(const RecordType *x) { defaultVisit(x); }
void ConstVisitor::visit(const RefType *x) { defaultVisit(x); }
void ConstVisitor::visit(const FuncType *x) { defaultVisit(x); }
void ConstVisitor::visit(const OptionalType *x) { defaultVisit(x); }
void ConstVisitor::visit(const PointerType *x) { defaultVisit(x); }
void ConstVisitor::visit(const GeneratorType *x) { defaultVisit(x); }
void ConstVisitor::visit(const VectorType *x) { defaultVisit(x); }
void ConstVisitor::visit(const UnionType *x) { defaultVisit(x); }
void ConstVisitor::visit(const dsl::CustomType *x) { defaultVisit(x); }

} // namespace util
} // namespace ir
} // namespace codon
