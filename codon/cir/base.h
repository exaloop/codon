// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "codon/cir/attribute.h"
#include "codon/cir/util/iterators.h"
#include "codon/cir/util/visitor.h"
#include "codon/util/common.h"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace codon {
namespace ir {

using id_t = std::int64_t;

class Func;
class Module;

/// Mixin class for IR nodes that need ids.
class IdMixin {
private:
  /// the global id counter
  static id_t currentId;

protected:
  /// the instance's id
  id_t id;

public:
  /// Resets the global id counter.
  static void resetId();

  IdMixin() : id(currentId++) {}

  /// @return the node's id.
  virtual id_t getId() const { return id; }
};

/// Base for named IR nodes.
class Node {
private:
  /// the node's name
  std::string name;
  /// the module
  Module *module = nullptr;
  /// a replacement, if set
  Node *replacement = nullptr;

protected:
  /// key-value attribute store
  std::unordered_map<int, std::unique_ptr<Attribute>> attributes;

public:
  // RTTI is implemented using a port of LLVM's Extensible RTTI
  // For more details, see
  // https://llvm.org/docs/HowToSetUpLLVMStyleRTTI.html#rtti-for-open-class-hierarchies
  static const char NodeId;

  /// Constructs a node.
  /// @param name the node's name
  explicit Node(std::string name = "") : name(std::move(name)) {}
  /// Constructs a node.
  /// @param name the node's name
  explicit Node(const Node &n);

  /// See LLVM documentation.
  static const void *nodeId() { return &NodeId; }
  /// See LLVM documentation.
  virtual const void *dynamicNodeId() const = 0;
  /// See LLVM documentation.
  virtual bool isConvertible(const void *other) const {
    if (hasReplacement())
      return getActual()->isConvertible(other);
    return other == nodeId();
  }
  /// See LLVM documentation.
  template <typename Target> bool is() const { return isConvertible(Target::nodeId()); }
  /// See LLVM documentation.
  template <typename Target> Target *as() {
    return isConvertible(Target::nodeId()) ? static_cast<Target *>(getActual())
                                           : nullptr;
  }
  /// See LLVM documentation.
  template <typename Target> const Target *as() const {
    return isConvertible(Target::nodeId()) ? static_cast<const Target *>(getActual())
                                           : nullptr;
  }

  /// @return the node's name
  const std::string &getName() const { return getActual()->name; }
  /// Sets the node's name
  /// @param n the new name
  void setName(std::string n) { getActual()->name = std::move(n); }

  /// Accepts visitors.
  /// @param v the visitor
  virtual void accept(util::Visitor &v) {}
  /// Accepts visitors.
  /// @param v the visitor
  virtual void accept(util::ConstVisitor &v) const {}

  /// Sets an attribute
  /// @param the attribute key
  /// @param value the attribute
  void setAttribute(std::unique_ptr<Attribute> value, int key) {
    getActual()->attributes[key] = std::move(value);
  }
  /// Sets an attribute
  /// @param value the attribute
  template <typename AttributeType>
  void setAttribute(std::unique_ptr<AttributeType> value) {
    setAttribute(std::move(value), AttributeType::AttributeID);
  }

  /// @param n attribute ID
  /// @return true if the attribute is in the store
  bool hasAttribute(int n) const {
    auto *actual = getActual();
    return actual->attributes.find(n) != actual->attributes.end();
  }
  /// @return true if the attribute is in the store
  template <typename AttributeType> bool hasAttribute() const {
    return hasAttribute(AttributeType::AttributeID);
  }

  /// Gets the appropriate attribute.
  /// @param key the attribute key
  Attribute *getAttribute(int key) {
    auto *actual = getActual();

    auto it = actual->attributes.find(key);
    return it != actual->attributes.end() ? it->second.get() : nullptr;
  }
  /// Gets the appropriate attribute.
  /// @param key the attribute key
  const Attribute *getAttribute(int key) const {
    auto *actual = getActual();

    auto it = actual->attributes.find(key);
    return it != actual->attributes.end() ? it->second.get() : nullptr;
  }
  /// Gets the appropriate attribute.
  /// @tparam AttributeType the return type
  template <typename AttributeType> AttributeType *getAttribute() {
    return static_cast<AttributeType *>(getAttribute(AttributeType::AttributeID));
  }
  /// Gets the appropriate attribute.
  /// @tparam AttributeType the return type
  template <typename AttributeType> const AttributeType *getAttribute() const {
    return static_cast<const AttributeType *>(getAttribute(AttributeType::AttributeID));
  }
  template <typename AttributeType> AttributeType *getAttribute(int key) {
    return static_cast<AttributeType *>(getAttribute(key));
  }
  template <typename AttributeType> const AttributeType *getAttribute(int key) const {
    return static_cast<const AttributeType *>(getAttribute(key));
  }
  void eraseAttribute(int key) { attributes.erase(key); }
  void cloneAttributesFrom(Node *n) { attributes = codon::clone(n->attributes); }

  /// @return iterator to the first attribute
  auto attributes_begin() {
    return util::map_key_adaptor(getActual()->attributes.begin());
  }
  /// @return iterator beyond the last attribute
  auto attributes_end() { return util::map_key_adaptor(getActual()->attributes.end()); }
  /// @return iterator to the first attribute
  auto attributes_begin() const {
    return util::const_map_key_adaptor(getActual()->attributes.begin());
  }
  /// @return iterator beyond the last attribute
  auto attributes_end() const {
    return util::const_map_key_adaptor(getActual()->attributes.end());
  }

  /// Helper to add source information.
  /// @param the source information
  void setSrcInfo(codon::SrcInfo s) {
    setAttribute(std::make_unique<SrcInfoAttribute>(std::move(s)));
  }
  /// @return the src info
  codon::SrcInfo getSrcInfo() const {
    auto a = getAttribute<SrcInfoAttribute>();
    return a ? a->info : codon::SrcInfo();
  }

  /// @return a text representation of a reference to the object
  virtual std::string referenceString() const { return getActual()->name; }

  /// @return the IR module
  Module *getModule() const { return getActual()->module; }
  /// Sets the module.
  /// @param m the new module
  void setModule(Module *m) { getActual()->module = m; }

  friend std::ostream &operator<<(std::ostream &os, const Node &a);

  bool hasReplacement() const { return replacement != nullptr; }

  /// @return a vector of all the node's children
  virtual std::vector<Value *> getUsedValues() { return {}; }
  /// @return a vector of all the node's children
  virtual std::vector<const Value *> getUsedValues() const { return {}; }
  /// Physically replaces all instances of a child value.
  /// @param id the id of the value to be replaced
  /// @param newValue the new value
  /// @return number of replacements
  virtual int replaceUsedValue(id_t id, Value *newValue) { return 0; }
  /// Physically replaces all instances of a child value.
  /// @param oldValue the old value
  /// @param newValue the new value
  /// @return number of replacements
  int replaceUsedValue(Value *old, Value *newValue);

  /// @return a vector of all the node's used types
  virtual std::vector<types::Type *> getUsedTypes() const { return {}; }
  /// Physically replaces all instances of a used type.
  /// @param name the name of the type being replaced
  /// @param newType the new type
  /// @return number of replacements
  virtual int replaceUsedType(const std::string &name, types::Type *newType) {
    return 0;
  }
  /// Physically replaces all instances of a used type.
  /// @param old the old type
  /// @param newType the new type
  /// @return number of replacements
  int replaceUsedType(types::Type *old, types::Type *newType);

  /// @return a vector of all the node's used variables
  virtual std::vector<Var *> getUsedVariables() { return {}; }
  /// @return a vector of all the node's used variables
  virtual std::vector<const Var *> getUsedVariables() const { return {}; }
  /// Physically replaces all instances of a used variable.
  /// @param id the id of the variable
  /// @param newType the new type
  /// @return number of replacements
  virtual int replaceUsedVariable(id_t id, Var *newVar) { return 0; }
  /// Physically replaces all instances of a used variable.
  /// @param old the old variable
  /// @param newVar the new variable
  /// @return number of replacements
  int replaceUsedVariable(Var *old, Var *newVar);

  template <typename, typename> friend class AcceptorExtend;
  template <typename> friend class ReplaceableNodeBase;

private:
  Node *getActual() { return replacement ? replacement->getActual() : this; }
  const Node *getActual() const {
    return replacement ? replacement->getActual() : this;
  }
};

template <typename Derived, typename Parent> class AcceptorExtend : public Parent {
public:
  using Parent::Parent;

  /// See LLVM documentation.
  static const void *nodeId() { return &Derived::NodeId; }
  /// See LLVM documentation.
  const void *dynamicNodeId() const override { return &Derived::NodeId; }
  /// See LLVM documentation.
  virtual bool isConvertible(const void *other) const override {
    if (Node::hasReplacement())
      return Node::getActual()->isConvertible(other);

    return other == nodeId() || Parent::isConvertible(other);
  }

  void accept(util::Visitor &v) override {
    if (Node::hasReplacement())
      Node::getActual()->accept(v);
    else
      v.visit(static_cast<Derived *>(this));
  }

  void accept(util::ConstVisitor &v) const override {
    if (Node::hasReplacement())
      Node::getActual()->accept(v);
    else
      v.visit(static_cast<const Derived *>(this));
  }
};

template <typename Derived>
class ReplaceableNodeBase : public AcceptorExtend<Derived, Node> {
private:
  /// true if the node can be lazily replaced
  bool replaceable = true;

public:
  using AcceptorExtend<Derived, Node>::AcceptorExtend;

  static const char NodeId;

  /// @return the logical value of the node
  Derived *getActual() {
    return Node::replacement ? static_cast<Derived *>(Node::replacement)->getActual()
                             : static_cast<Derived *>(this);
  }

  /// @return the logical value of the node
  const Derived *getActual() const {
    return Node::replacement
               ? static_cast<const Derived *>(Node::replacement)->getActual()
               : static_cast<const Derived *>(this);
  }

  /// Lazily replaces all instances of the node.
  /// @param v the new value
  void replaceAll(Derived *v) {
    seqassertn(replaceable, "node {} not replaceable", *v);
    Node::replacement = v;
  }

  /// @return true if the object can be replaced
  bool isReplaceable() const { return replaceable; }
  /// Sets the object's replaceable flag.
  /// @param v the new value
  void setReplaceable(bool v = true) { replaceable = v; }
};

template <typename Derived> const char ReplaceableNodeBase<Derived>::NodeId = 0;

template <typename Desired> Desired *cast(Node *other) {
  return other != nullptr ? other->as<Desired>() : nullptr;
}

template <typename Desired> const Desired *cast(const Node *other) {
  return other != nullptr ? other->as<Desired>() : nullptr;
}

template <typename Desired> bool isA(Node *other) {
  return other && other->is<Desired>();
}

template <typename Desired> bool isA(const Node *other) {
  return other && other->is<Desired>();
}

} // namespace ir
} // namespace codon

template <> struct fmt::formatter<codon::ir::Node> : fmt::ostream_formatter {};
