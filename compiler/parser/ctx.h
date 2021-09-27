/*
 * context.h --- Base Context class for tracking identifiers in a code.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#pragma once

#include <deque>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"

namespace seq {
namespace ast {

/**
 * A variable table (transformation context).
 * Base class that holds a list of existing identifiers and their block hierarchy.
 * @tparam T Variable type.
 */
template <typename T> class Context : public std::enable_shared_from_this<Context<T>> {
protected:
  typedef unordered_map<string, std::deque<std::pair<int, shared_ptr<T>>>> Map;
  /// Maps a identifier to a stack of objects that share the same identifier.
  /// Each object is represented by a nesting level and a pointer to that object.
  /// Top of the stack is the current block; the bottom is the outer-most block.
  /// Stack is represented as std::deque to allow iteration and access to the outer-most
  /// block.
  Map map;
  /// Stack of blocks and their corresponding identifiers. Top of the stack is the
  /// current block.
  std::deque<vector<string>> stack;

private:
  /// Set of current context flags.
  unordered_set<string> flags;
  /// The absolute path of the current module.
  string filename;

public:
  explicit Context(string filename) : filename(move(filename)) {
    /// Add a top-level block to the stack.
    stack.push_front(vector<string>());
  }
  virtual ~Context() = default;

  /// Add an object to the top of the stack.
  void add(const string &name, shared_ptr<T> var) {
    seqassert(!name.empty(), "adding an empty identifier");
    map[name].push_front({stack.size(), move(var)});
    stack.front().push_back(name);
  }
  /// Add an object to the top of the previous block.
  void addPrevBlock(const string &name, shared_ptr<T> var) {
    seqassert(!name.empty(), "adding an empty identifier");
    seqassert(stack.size() > 1, "adding an empty identifier");
    auto &m = map[name];
    int pos = 0;
    /// Make sure to add it to the appropriate place in the Map
    /// (because each stack is not stacked itself, we have to use pos to find top-level
    /// position).
    while (pos < m.size() && m[pos].first == stack.size())
      pos++;
    m.insert(m.begin() + pos, {stack.size() - 1, move(var)});
    stack[1].push_back(name);
  }
  /// Add an object to the top-level (bottom of the stack).
  void addToplevel(const string &name, shared_ptr<T> var) {
    seqassert(!name.empty(), "adding an empty identifier");
    auto &m = map[name];
    int pos = m.size();
    /// Make sure to add it to the appropriate place in the Map
    /// (because each stack is not stacked itself, we have to use pos to find top-level
    /// position).
    while (pos > 0 && m[pos - 1].first == 1)
      pos--;
    m.insert(m.begin() + pos, {1, move(var)});
    stack.back().push_back(name); // add to the latest "level"
  }
  /// Remove the top-most object with a given identifier.
  void remove(const string &name) {
    removeFromMap(name);
    for (auto &s : stack) {
      auto i = std::find(s.begin(), s.end(), name);
      if (i != s.end()) {
        s.erase(i);
        return;
      }
    }
    seqassert(false, "cannot find {} in the stack", name);
  }
  /// Return a top-most object with a given identifier or nullptr if it does not exist.
  virtual shared_ptr<T> find(const string &name) const {
    auto it = map.find(name);
    return it != map.end() ? it->second.front().second : nullptr;
  }
  /// Add a new block (i.e. adds a stack level).
  void addBlock() { stack.push_front(vector<string>()); }
  /// Remove the top-most block and all variables it holds.
  void popBlock() {
    for (auto &name : stack.front())
      removeFromMap(name);
    stack.pop_front();
  }

  /// True if only the top-level block is present.
  bool isToplevel() const { return stack.size() == 1; }
  /// The absolute path of a current module.
  string getFilename() const { return filename; }
  /// Sets the absolute path of a current module.
  void setFilename(string file) { filename = move(file); }

  /// Convenience functions to allow range-based for loops over a context.
  typename Map::iterator begin() { return map.begin(); }
  typename Map::iterator end() { return map.end(); }

  /// Pretty-prints the current context state.
  virtual void dump() {}

private:
  /// Remove an identifier from the map only.
  void removeFromMap(const string &name) {
    auto i = map.find(name);
    seqassert(!(i == map.end() || !i->second.size()),
              "identifier {} not found in the map", name);
    i->second.pop_front();
    if (!i->second.size())
      map.erase(name);
  }
};

} // namespace ast
} // namespace seq
