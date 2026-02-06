// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <ostream>
#include <string>

#include "codon/cir/base.h"

namespace codon::ast {

using ir::cast;

// Forward declarations
struct Cache;
struct ASTVisitor;

struct ASTNode : public ir::Node {
  static const char NodeId;
  using ir::Node::Node;

  /// See LLVM documentation.
  static const void *nodeId() { return &NodeId; }
  const void *dynamicNodeId() const override { return &NodeId; }
  /// See LLVM documentation.
  virtual bool isConvertible(const void *other) const override {
    return other == nodeId() || ir::Node::isConvertible(other);
  }

  Cache *cache = nullptr;

  ASTNode() = default;
  ASTNode(const ASTNode &) = default;
  virtual ~ASTNode() = default;

  /// Convert a node to an S-expression.
  virtual std::string toString(int) const = 0;
  virtual std::string toString() const { return toString(-1); }

  /// Deep copy a node.
  virtual ASTNode *clone(bool clean) const = 0;
  ASTNode *clone() const { return clone(false); }

  using ir::Node::accept;
  /// Accept an AST visitor.
  virtual void accept(ASTVisitor &visitor) {}

  /// Allow pretty-printing to C++ streams.
  friend std::ostream &operator<<(std::ostream &out, const ASTNode &expr) {
    return out << expr.toString();
  }

  void setAttribute(int key, std::unique_ptr<ir::Attribute> value) {
    attributes[key] = std::move(value);
  }
  void setAttribute(int key, const std::string &value) {
    attributes[key] = std::make_unique<ir::StringValueAttribute>(value);
  }
  void setAttribute(int key, int64_t value) {
    attributes[key] = std::make_unique<ir::IntValueAttribute>(value);
  }
  void setAttribute(int key) { attributes[key] = std::make_unique<ir::Attribute>(); }

  inline decltype(auto) members() {
    int a = 0;
    return std::tie(a);
  }
};

template <class... TA> void E(error::Error e, ASTNode *o, const TA &...args) {
  E(e, o->getSrcInfo(), args...);
}
template <class... TA> void E(error::Error e, const ASTNode &o, const TA &...args) {
  E(e, o.getSrcInfo(), args...);
}

template <typename Derived, typename Parent> class AcceptorExtend : public Parent {
public:
  using Parent::Parent;

  /// See LLVM documentation.
  static const void *nodeId() { return &Derived::NodeId; }
  const void *dynamicNodeId() const override { return &Derived::NodeId; }
  /// See LLVM documentation.
  virtual bool isConvertible(const void *other) const override {
    return other == nodeId() || Parent::isConvertible(other);
  }
};

template <class T> struct Items {
  explicit Items(std::vector<T> items) : items(std::move(items)) {}
  const T &operator[](size_t i) const { return items[i]; }
  T &operator[](size_t i) { return items[i]; }
  auto begin() { return items.begin(); }
  auto end() { return items.end(); }
  auto begin() const { return items.begin(); }
  auto end() const { return items.end(); }
  auto size() const { return items.size(); }
  bool empty() const { return items.empty(); }
  const T &front() const { return items.front(); }
  const T &back() const { return items.back(); }
  T &front() { return items.front(); }
  T &back() { return items.back(); }

protected:
  std::vector<T> items;
};

} // namespace codon::ast

template <typename T>
struct fmt::formatter<T,
                      std::enable_if_t<std::is_base_of_v<codon::ast::ASTNode, T>, char>>
    : fmt::ostream_formatter {};
