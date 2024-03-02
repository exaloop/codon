// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <deque>
#include <list>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"

namespace codon::ast {

/**
 * A variable table (transformation context).
 * Base class that holds a list of existing identifiers and their block hierarchy.
 * @tparam T Variable type.
 */
template <typename T> class Context : public std::enable_shared_from_this<Context<T>> {
public:
  using Item = std::shared_ptr<T>;

protected:
  using Map = std::unordered_map<std::string, std::list<Item>>;
  /// Maps a identifier to a stack of objects that share the same identifier.
  /// Each object is represented by a nesting level and a pointer to that object.
  /// Top of the stack is the current block; the bottom is the outer-most block.
  /// Stack is represented as std::deque to allow iteration and access to the outer-most
  /// block.
  Map map;
  /// Stack of blocks and their corresponding identifiers. Top of the stack is the
  /// current block.
  std::deque<std::list<std::string>> stack;

private:
  /// Set of current context flags.
  std::unordered_set<std::string> flags;
  /// The absolute path of the current module.
  std::string filename;
  /// SrcInfo stack used for obtaining source information of the current expression.
  std::vector<SrcInfo> srcInfos;

public:
  explicit Context(std::string filename) : filename(std::move(filename)) {
    /// Add a top-level block to the stack.
    stack.push_front(std::list<std::string>());
  }
  virtual ~Context() = default;

  /// Add an object to the top of the stack.
  virtual void add(const std::string &name, const Item &var) {
    seqassertn(!name.empty(), "adding an empty identifier");
    map[name].push_front(var);
    stack.front().push_back(name);
  }
  /// Remove the top-most object with a given identifier.
  void remove(const std::string &name) {
    removeFromMap(name);
    for (auto &s : stack) {
      auto i = std::find(s.begin(), s.end(), name);
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
  /// Add a new block (i.e. adds a stack level).
  virtual void addBlock() { stack.push_front(std::list<std::string>()); }
  /// Remove the top-most block and all variables it holds.
  virtual void popBlock() {
    for (auto &name : stack.front())
      removeFromMap(name);
    stack.pop_front();
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

private:
  /// Remove an identifier from the map only.
  void removeFromMap(const std::string &name) {
    auto i = map.find(name);
    if (i == map.end())
      return;
    seqassertn(i->second.size(), "identifier {} not found in the map", name);
    i->second.pop_front();
    if (!i->second.size())
      map.erase(name);
  }

public:
  /* SrcInfo helpers */
  void pushSrcInfo(SrcInfo s) { srcInfos.emplace_back(std::move(s)); }
  void popSrcInfo() { srcInfos.pop_back(); }
  SrcInfo getSrcInfo() const { return srcInfos.back(); }
};

} // namespace codon::ast
