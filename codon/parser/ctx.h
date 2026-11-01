// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "codon/parser/ast.h"

namespace codon::ast {

/**
 * A variable table (transformation context).
 * Base class that holds a list of existing identifiers and their block hierarchy.
 * @tparam T Variable type.
 */
template <typename T> class Context : public std::enable_shared_from_this<Context<T>> {
public:
  using Item = std::shared_ptr<T>;
  using BindingStack = llvm::SmallVector<Item, 2>;

protected:
  using Map = std::unordered_map<std::string, BindingStack>;
  /// Maps a identifier to a stack of objects that share the same identifier.
  /// Each object is represented by a nesting level and a pointer to that object.
  /// Top of the stack is the current block; the bottom is the outer-most block.
  /// Stack is represented as std::deque to allow iteration and access to the outer-most
  /// block.
  Map map;
  /// Stack of blocks and their corresponding identifiers. Top of the stack is the
  /// current block.
  std::deque<std::vector<std::string>> stack;

private:
  /// Set of current context flags.
  std::unordered_set<std::string> flags;
  /// The absolute path of the current module.
  std::string filename;
  /// SrcInfo stack used for obtaining source information of the current expression.
  std::vector<ASTNode *> nodeStack;

public:
  explicit Context(std::string filename) : filename(std::move(filename)) {
    /// Add a top-level block to the stack.
    stack.emplace_front();
  }
  virtual ~Context() = default;
  Context(const Context &) = default;
  Context &operator=(const Context &) = default;
  Context(Context &&) noexcept = default;
  Context &operator=(Context &&) noexcept = default;

  /// Record only bindings changed during a speculative realization. Context stacks
  /// and other inference state are saved separately by TypeContext.
  class BindingCheckpoint {
    Context *context;
    BindingCheckpoint *previous;
    std::unordered_map<std::string, std::optional<BindingStack>> bindings;
    std::function<void()> saveState;
    bool prepared = false;
    friend class Context;

  public:
    explicit BindingCheckpoint(Context *context, std::function<void()> saveState = {})
        : context(context), saveState(std::move(saveState)) {
      auto &entry = checkpoints[context];
      previous = entry;
      entry = this;
    }
    BindingCheckpoint(const BindingCheckpoint &) = delete;
    void prepare() {
      if (!prepared) {
        prepared = true;
        if (saveState)
          saveState();
      }
    }
    ~BindingCheckpoint() {
      if (previous)
        checkpoints[context] = previous;
      else
        checkpoints.erase(context);
    }
    template <typename Preserve> void rollback(Preserve preserve) {
      for (auto &[name, saved] : bindings) {
        if (saved) {
          context->map[name] = std::move(*saved);
        } else {
          auto current = context->map.find(name);
          if (current != context->map.end() && !current->second.empty() &&
              preserve(current->second.back())) {
            auto item = current->second.back();
            current->second.clear();
            current->second.push_back(std::move(item));
          } else {
            context->map.erase(name);
          }
        }
      }
    }
  };

protected:
  inline static thread_local std::unordered_map<Context *, BindingCheckpoint *>
      checkpoints;
  void recordBinding(const std::string &name) {
    if (checkpoints.empty())
      return;
    auto found = checkpoints.find(this);
    if (found == checkpoints.end())
      return;
    for (auto *checkpoint = found->second; checkpoint;
         checkpoint = checkpoint->previous) {
      checkpoint->prepare();
      if (checkpoint->bindings.count(name))
        continue;
      auto current = map.find(name);
      if (current == map.end())
        checkpoint->bindings.emplace(name, std::nullopt);
      else
        checkpoint->bindings.emplace(name, current->second);
    }
  }

public:
  void prepareCheckpoint() {
    if (checkpoints.empty())
      return;
    auto found = checkpoints.find(this);
    if (found != checkpoints.end())
      for (auto *checkpoint = found->second; checkpoint;
           checkpoint = checkpoint->previous)
        checkpoint->prepare();
  }

  /// Add an object to the top of the stack.
  virtual void add(const std::string &name, const Item &var) {
    seqassertn(!name.empty(), "adding an empty identifier");
    recordBinding(name);
    auto &bindings = map[name];
    bindings.insert(bindings.begin(), var);
    stack.front().push_back(name);
  }
  /// Remove the top-most object with a given identifier.
  void remove(const std::string &name) {
    removeFromMap(name);
    for (auto &s : stack) {
      auto i = std::ranges::find(s, name);
      if (i != s.end()) {
        s.erase(i);
        return;
      }
    }
  }
  /// Return a top-most object with a given identifier or nullptr if it does not exist.
  virtual Item find(const std::string &name) const {
    auto it = map.find(name);
    return it != map.end() ? it->second.front() : nullptr;
  }
  /// Return all objects that share a common identifier or nullptr if it does not exist.
  virtual BindingStack *find_all(const std::string &name) {
    auto it = map.find(name);
    return it != map.end() ? &(it->second) : nullptr;
  }
  /// Add a new block (i.e. adds a stack level).
  virtual void addBlock() {
    if (!checkpoints.empty())
      prepareCheckpoint();
    stack.emplace_front();
  }
  /// Remove the top-most block and all variables it holds.
  virtual void popBlock() {
    for (auto &name : stack.front())
      removeFromMap(name);
    stack.pop_front();
  }
  const std::vector<std::string> &getBlock() const { return stack.front(); }

  void removeFromTopStack(const std::string &name) {
    auto it = std::ranges::find(stack.front(), name);
    if (it != stack.front().end())
      stack.front().erase(it);
  }

  /// The absolute path of a current module.
  std::string getFilename() const { return filename; }
  /// Sets the absolute path of a current module.
  void setFilename(std::string file) { filename = std::move(file); }

  /// Convenience functions to allow range-based for loops over a context.
  typename Map::iterator begin() { return map.begin(); }
  typename Map::iterator end() { return map.end(); }

  /// Pretty-prints the current context state.
  virtual void dump() {}

protected:
  /// Remove an identifier from the map only.
  virtual void removeFromMap(const std::string &name) {
    auto i = map.find(name);
    if (i == map.end())
      return;
    recordBinding(name);
    seqassertn(i->second.size(), "identifier {} not found in the map", name);
    i->second.erase(i->second.begin());
    if (!i->second.size())
      map.erase(name);
  }

public:
  /* SrcInfo helpers */
  void pushNode(ASTNode *n) { nodeStack.emplace_back(n); }
  void popNode() { nodeStack.pop_back(); }
  ASTNode *getLastNode() const { return nodeStack.back(); }
  ASTNode *getParentNode() const {
    assert(nodeStack.size() > 1);
    return nodeStack[nodeStack.size() - 2];
  }
  SrcInfo getSrcInfo() const { return nodeStack.back()->getSrcInfo(); }
  size_t getStackSize() const { return stack.size(); }
};

} // namespace codon::ast
