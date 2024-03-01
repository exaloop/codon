// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <list>
#include <vector>

#include "codon/cir/base.h"
#include "codon/cir/transform/parallel/schedule.h"
#include "codon/cir/value.h"
#include "codon/cir/var.h"

namespace codon {
namespace ir {

/// Base for flows, which represent control flow.
class Flow : public AcceptorExtend<Flow, Value> {
public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

protected:
  types::Type *doGetType() const final;
};

/// Flow that contains a series of flows or instructions.
class SeriesFlow : public AcceptorExtend<SeriesFlow, Flow> {
private:
  std::list<Value *> series;

public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

  /// @return an iterator to the first instruction/flow
  auto begin() { return series.begin(); }
  /// @return an iterator beyond the last instruction/flow
  auto end() { return series.end(); }
  /// @return an iterator to the first instruction/flow
  auto begin() const { return series.begin(); }
  /// @return an iterator beyond the last instruction/flow
  auto end() const { return series.end(); }

  /// @return a pointer to the first instruction/flow
  Value *front() { return series.front(); }
  /// @return a pointer to the last instruction/flow
  Value *back() { return series.back(); }
  /// @return a pointer to the first instruction/flow
  const Value *front() const { return series.front(); }
  /// @return a pointer to the last instruction/flow
  const Value *back() const { return series.back(); }

  /// Inserts an instruction/flow at the given position.
  /// @param pos the position
  /// @param v the flow or instruction
  /// @return an iterator to the newly added instruction/flow
  template <typename It> auto insert(It pos, Value *v) { return series.insert(pos, v); }
  /// Appends an instruction/flow.
  /// @param f the flow or instruction
  void push_back(Value *f) { series.push_back(f); }

  /// Erases the item at the supplied position.
  /// @param pos the position
  /// @return the iterator beyond the removed flow or instruction
  template <typename It> auto erase(It pos) { return series.erase(pos); }

protected:
  std::vector<Value *> doGetUsedValues() const override {
    return std::vector<Value *>(series.begin(), series.end());
  }
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Flow representing a while loop.
class WhileFlow : public AcceptorExtend<WhileFlow, Flow> {
private:
  /// the condition
  Value *cond;
  /// the body
  Value *body;

public:
  static const char NodeId;

  /// Constructs a while loop.
  /// @param cond the condition
  /// @param body the body
  /// @param name the flow's name
  WhileFlow(Value *cond, Flow *body, std::string name = "")
      : AcceptorExtend(std::move(name)), cond(cond), body(body) {}

  /// @return the condition
  Value *getCond() { return cond; }
  /// @return the condition
  const Value *getCond() const { return cond; }
  /// Sets the condition.
  /// @param c the new condition
  void setCond(Value *c) { cond = c; }

  /// @return the body
  Flow *getBody() { return cast<Flow>(body); }
  /// @return the body
  const Flow *getBody() const { return cast<Flow>(body); }
  /// Sets the body.
  /// @param f the new value
  void setBody(Flow *f) { body = f; }

protected:
  std::vector<Value *> doGetUsedValues() const override { return {cond, body}; }

  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Flow representing a for loop.
class ForFlow : public AcceptorExtend<ForFlow, Flow> {
private:
  /// the iterator
  Value *iter;
  /// the body
  Value *body;
  /// the variable
  Var *var;
  /// parallel loop schedule, or null if none
  std::unique_ptr<transform::parallel::OMPSched> schedule;

public:
  static const char NodeId;

  /// Constructs a for loop.
  /// @param iter the iterator
  /// @param body the body
  /// @param var the variable
  /// @param name the flow's name
  ForFlow(Value *iter, Flow *body, Var *var,
          std::unique_ptr<transform::parallel::OMPSched> schedule = {},
          std::string name = "")
      : AcceptorExtend(std::move(name)), iter(iter), body(body), var(var),
        schedule(std::move(schedule)) {}

  /// @return the iter
  Value *getIter() { return iter; }
  /// @return the iter
  const Value *getIter() const { return iter; }
  /// Sets the iter.
  /// @param f the new iter
  void setIter(Value *f) { iter = f; }

  /// @return the body
  Flow *getBody() { return cast<Flow>(body); }
  /// @return the body
  const Flow *getBody() const { return cast<Flow>(body); }
  /// Sets the body.
  /// @param f the new body
  void setBody(Flow *f) { body = f; }

  /// @return the var
  Var *getVar() { return var; }
  /// @return the var
  const Var *getVar() const { return var; }
  /// Sets the var.
  /// @param c the new var
  void setVar(Var *c) { var = c; }

  /// @return true if parallel
  bool isParallel() const { return bool(schedule); }
  /// Sets parallel status.
  /// @param a true if parallel
  void setParallel(bool a = true) {
    if (a)
      schedule = std::make_unique<transform::parallel::OMPSched>();
    else
      schedule = std::unique_ptr<transform::parallel::OMPSched>();
  }

  /// @return the parallel loop schedule, or null if none
  transform::parallel::OMPSched *getSchedule() { return schedule.get(); }
  /// @return the parallel loop schedule, or null if none
  const transform::parallel::OMPSched *getSchedule() const { return schedule.get(); }
  /// Sets the parallel loop schedule
  /// @param s the schedule string (e.g. OpenMP pragma)
  void setSchedule(std::unique_ptr<transform::parallel::OMPSched> s) {
    schedule = std::move(s);
  }

protected:
  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;

  std::vector<Var *> doGetUsedVariables() const override { return {var}; }
  int doReplaceUsedVariable(id_t id, Var *newVar) override;
};

/// Flow representing an imperative for loop.
class ImperativeForFlow : public AcceptorExtend<ImperativeForFlow, Flow> {
private:
  /// the initial value
  Value *start;
  /// the step value
  int64_t step;
  /// the end value
  Value *end;
  /// the body
  Value *body;
  /// the variable, must be integer type
  Var *var;
  /// parallel loop schedule, or null if none
  std::unique_ptr<transform::parallel::OMPSched> schedule;

public:
  static const char NodeId;

  /// Constructs an imperative for loop.
  /// @param body the body
  /// @param start the start value
  /// @param step the step value
  /// @param end the end value
  /// @param var the end variable, must be integer
  /// @param name the flow's name
  ImperativeForFlow(Value *start, int64_t step, Value *end, Flow *body, Var *var,
                    std::unique_ptr<transform::parallel::OMPSched> schedule = {},
                    std::string name = "")
      : AcceptorExtend(std::move(name)), start(start), step(step), end(end), body(body),
        var(var), schedule(std::move(schedule)) {}

  /// @return the start value
  Value *getStart() const { return start; }
  /// Sets the start value.
  /// @param v the new value
  void setStart(Value *val) { start = val; }

  /// @return the step value
  int64_t getStep() const { return step; }
  /// Sets the step value.
  /// @param v the new value
  void setStep(int64_t val) { step = val; }

  /// @return the end value
  Value *getEnd() const { return end; }
  /// Sets the end value.
  /// @param v the new value
  void setEnd(Value *val) { end = val; }

  /// @return the body
  Flow *getBody() { return cast<Flow>(body); }
  /// @return the body
  const Flow *getBody() const { return cast<Flow>(body); }
  /// Sets the body.
  /// @param f the new body
  void setBody(Flow *f) { body = f; }

  /// @return the var
  Var *getVar() { return var; }
  /// @return the var
  const Var *getVar() const { return var; }
  /// Sets the var.
  /// @param c the new var
  void setVar(Var *c) { var = c; }

  /// @return true if parallel
  bool isParallel() const { return bool(schedule); }
  /// Sets parallel status.
  /// @param a true if parallel
  void setParallel(bool a = true) {
    if (a)
      schedule = std::make_unique<transform::parallel::OMPSched>();
    else
      schedule = std::unique_ptr<transform::parallel::OMPSched>();
  }

  /// @return the parallel loop schedule, or null if none
  transform::parallel::OMPSched *getSchedule() { return schedule.get(); }
  /// @return the parallel loop schedule, or null if none
  const transform::parallel::OMPSched *getSchedule() const { return schedule.get(); }
  /// Sets the parallel loop schedule
  /// @param s the schedule string (e.g. OpenMP pragma)
  void setSchedule(std::unique_ptr<transform::parallel::OMPSched> s) {
    schedule = std::move(s);
  }

protected:
  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;

  std::vector<Var *> doGetUsedVariables() const override { return {var}; }
  int doReplaceUsedVariable(id_t id, Var *newVar) override;
};

/// Flow representing an if statement.
class IfFlow : public AcceptorExtend<IfFlow, Flow> {
private:
  /// the condition
  Value *cond;
  /// the true branch
  Value *trueBranch;
  /// the false branch
  Value *falseBranch;

public:
  static const char NodeId;

  /// Constructs an if.
  /// @param cond the condition
  /// @param trueBranch the true branch
  /// @param falseBranch the false branch
  /// @param name the flow's name
  IfFlow(Value *cond, Flow *trueBranch, Flow *falseBranch = nullptr,
         std::string name = "")
      : AcceptorExtend(std::move(name)), cond(cond), trueBranch(trueBranch),
        falseBranch(falseBranch) {}

  /// @return the true branch
  Flow *getTrueBranch() { return cast<Flow>(trueBranch); }
  /// @return the true branch
  const Flow *getTrueBranch() const { return cast<Flow>(trueBranch); }
  /// Sets the true branch.
  /// @param f the new true branch
  void setTrueBranch(Flow *f) { trueBranch = f; }

  /// @return the false branch
  Flow *getFalseBranch() { return cast<Flow>(falseBranch); }
  /// @return the false branch
  const Flow *getFalseBranch() const { return cast<Flow>(falseBranch); }
  /// Sets the false.
  /// @param f the new false
  void setFalseBranch(Flow *f) { falseBranch = f; }

  /// @return the condition
  Value *getCond() { return cond; }
  /// @return the condition
  const Value *getCond() const { return cond; }
  /// Sets the condition.
  /// @param c the new condition
  void setCond(Value *c) { cond = c; }

protected:
  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Flow representing a try-catch statement.
class TryCatchFlow : public AcceptorExtend<TryCatchFlow, Flow> {
public:
  /// Class representing a catch clause.
  class Catch {
  private:
    /// the handler
    Value *handler;
    /// the catch type, may be nullptr
    types::Type *type;
    /// the catch variable, may be nullptr
    Var *catchVar;

  public:
    explicit Catch(Flow *handler, types::Type *type = nullptr, Var *catchVar = nullptr)
        : handler(handler), type(type), catchVar(catchVar) {}

    /// @return the handler
    Flow *getHandler() { return cast<Flow>(handler); }
    /// @return the handler
    const Flow *getHandler() const { return cast<Flow>(handler); }
    /// Sets the handler.
    /// @param h the new value
    void setHandler(Flow *h) { handler = h; }

    /// @return the catch type, may be nullptr
    types::Type *getType() const { return type; }
    /// Sets the catch type.
    /// @param t the new type, nullptr for catch all
    void setType(types::Type *t) { type = t; }

    /// @return the variable, may be nullptr
    Var *getVar() { return catchVar; }
    /// @return the variable, may be nullptr
    const Var *getVar() const { return catchVar; }
    /// Sets the variable.
    /// @param v the new value, may be nullptr
    void setVar(Var *v) { catchVar = v; }
  };

private:
  /// the catch clauses
  std::list<Catch> catches;

  /// the body
  Value *body;
  /// the finally, may be nullptr
  Value *finally;

public:
  static const char NodeId;

  /// Constructs an try-catch.
  /// @param name the's name
  /// @param body the body
  /// @param finally the finally
  explicit TryCatchFlow(Flow *body, Flow *finally = nullptr, std::string name = "")
      : AcceptorExtend(std::move(name)), body(body), finally(finally) {}

  /// @return the body
  Flow *getBody() { return cast<Flow>(body); }
  /// @return the body
  const Flow *getBody() const { return cast<Flow>(body); }
  /// Sets the body.
  /// @param f the new
  void setBody(Flow *f) { body = f; }

  /// @return the finally
  Flow *getFinally() { return cast<Flow>(finally); }
  /// @return the finally
  const Flow *getFinally() const { return cast<Flow>(finally); }
  /// Sets the finally.
  /// @param f the new
  void setFinally(Flow *f) { finally = f; }

  /// @return an iterator to the first catch
  auto begin() { return catches.begin(); }
  /// @return an iterator beyond the last catch
  auto end() { return catches.end(); }
  /// @return an iterator to the first catch
  auto begin() const { return catches.begin(); }
  /// @return an iterator beyond the last catch
  auto end() const { return catches.end(); }

  /// @return a reference to the first catch
  auto &front() { return catches.front(); }
  /// @return a reference to the last catch
  auto &back() { return catches.back(); }
  /// @return a reference to the first catch
  auto &front() const { return catches.front(); }
  /// @return a reference to the last catch
  auto &back() const { return catches.back(); }

  /// Inserts a catch at the given position.
  /// @param pos the position
  /// @param v the catch
  /// @return an iterator to the newly added catch
  template <typename It> auto insert(It pos, Catch v) { return catches.insert(pos, v); }

  /// Appends a catch.
  /// @param v the catch
  void push_back(Catch v) { catches.push_back(v); }

  /// Emplaces a catch.
  /// @tparam Args the catch constructor args
  template <typename... Args> void emplace_back(Args &&...args) {
    catches.emplace_back(std::forward<Args>(args)...);
  }

  /// Erases a catch at the given position.
  /// @param pos the position
  /// @return the iterator beyond the erased catch
  template <typename It> auto erase(It pos) { return catches.erase(pos); }

protected:
  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;

  std::vector<types::Type *> doGetUsedTypes() const override;
  int doReplaceUsedType(const std::string &name, types::Type *newType) override;

  std::vector<Var *> doGetUsedVariables() const override;
  int doReplaceUsedVariable(id_t id, Var *newVar) override;
};

/// Flow that represents a pipeline. Pipelines with only function
/// stages are expressions and have a concrete type. Pipelines with
/// generator stages are not expressions and have no type. This
/// representation allows for stages that output generators but do
/// not get explicitly iterated in the pipeline, since generator
/// stages are denoted by a separate flag.
class PipelineFlow : public AcceptorExtend<PipelineFlow, Flow> {
public:
  /// Represents a single stage in a pipeline.
  class Stage {
  private:
    /// the function being (partially) called in this stage
    Value *callee;
    /// the function arguments, where null represents where
    /// previous pipeline output should go
    std::vector<Value *> args;
    /// true if this stage is a generator
    bool generator;
    /// true if this stage is marked parallel
    bool parallel;

  public:
    /// Constructs a pipeline stage.
    /// @param callee the function being called
    /// @param args call arguments, with exactly one null entry
    /// @param generator whether this stage is a generator stage
    /// @param parallel whether this stage is parallel
    Stage(Value *callee, std::vector<Value *> args, bool generator, bool parallel)
        : callee(callee), args(std::move(args)), generator(generator),
          parallel(parallel) {}

    /// @return an iterator to the first argument
    auto begin() { return args.begin(); }
    /// @return an iterator beyond the last argument
    auto end() { return args.end(); }
    /// @return an iterator to the first argument
    auto begin() const { return args.begin(); }
    /// @return an iterator beyond the last argument
    auto end() const { return args.end(); }

    /// @return a pointer to the first argument
    Value *front() { return args.front(); }
    /// @return a pointer to the last argument
    Value *back() { return args.back(); }
    /// @return a pointer to the first argument
    const Value *front() const { return args.front(); }
    /// @return a pointer to the last argument
    const Value *back() const { return args.back(); }

    /// Inserts an argument.
    /// @param pos the position
    /// @param v the argument
    /// @return an iterator to the newly added argument
    template <typename It> auto insert(It pos, Value *v) { return args.insert(pos, v); }
    /// Appends an argument.
    /// @param v the argument
    void push_back(Value *v) { args.push_back(v); }

    /// Erases the item at the supplied position.
    /// @param pos the position
    /// @return the iterator beyond the removed argument
    template <typename It> auto erase(It pos) { return args.erase(pos); }

    /// Sets the called function.
    /// @param c the callee
    void setCallee(Value *c) { callee = c; }
    /// @return the called function
    Value *getCallee() { return callee; }
    /// @return the called function
    const Value *getCallee() const { return callee; }

    /// Sets the stage's generator flag.
    /// @param v the new value
    void setGenerator(bool v = true) { generator = v; }
    /// @return whether this stage is a generator stage
    bool isGenerator() const { return generator; }
    /// Sets the stage's parallel flag.
    /// @param v the new value
    void setParallel(bool v = true) { parallel = v; }
    /// @return whether this stage is parallel
    bool isParallel() const { return parallel; }
    /// @return the output type of this stage
    types::Type *getOutputType() const;
    /// @return the output element type of this stage
    types::Type *getOutputElementType() const;

    friend class PipelineFlow;
  };

private:
  /// pipeline stages
  std::list<Stage> stages;

public:
  static const char NodeId;

  /// Constructs a pipeline flow.
  /// @param stages vector of pipeline stages
  /// @param name the name
  explicit PipelineFlow(std::vector<Stage> stages = {}, std::string name = "")
      : AcceptorExtend(std::move(name)), stages(stages.begin(), stages.end()) {}

  /// @return an iterator to the first stage
  auto begin() { return stages.begin(); }
  /// @return an iterator beyond the last stage
  auto end() { return stages.end(); }
  /// @return an iterator to the first stage
  auto begin() const { return stages.begin(); }
  /// @return an iterator beyond the last stage
  auto end() const { return stages.end(); }

  /// @return a pointer to the first stage
  Stage &front() { return stages.front(); }
  /// @return a pointer to the last stage
  Stage &back() { return stages.back(); }
  /// @return a pointer to the first stage
  const Stage &front() const { return stages.front(); }
  /// @return a pointer to the last stage
  const Stage &back() const { return stages.back(); }

  /// Inserts a stage
  /// @param pos the position
  /// @param v the stage
  /// @return an iterator to the newly added stage
  template <typename It> auto insert(It pos, Stage v) { return stages.insert(pos, v); }
  /// Appends an stage.
  /// @param v the stage
  void push_back(Stage v) { stages.push_back(std::move(v)); }

  /// Erases the item at the supplied position.
  /// @param pos the position
  /// @return the iterator beyond the removed stage
  template <typename It> auto erase(It pos) { return stages.erase(pos); }

  /// Emplaces a stage.
  /// @param args the args
  template <typename... Args> void emplace_back(Args &&...args) {
    stages.emplace_back(std::forward<Args>(args)...);
  }

protected:
  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

} // namespace ir
} // namespace codon
