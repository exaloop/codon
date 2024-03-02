// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "codon/util/common.h"

namespace codon {
namespace ir {

class Func;
class Value;

namespace util {
class CloneVisitor;
}

/// Base for CIR attributes.
struct Attribute {
  virtual ~Attribute() noexcept = default;

  /// @return true if the attribute should be propagated across clones
  virtual bool needsClone() const { return true; }

  friend std::ostream &operator<<(std::ostream &os, const Attribute &a) {
    return a.doFormat(os);
  }

  /// @return a clone of the attribute
  virtual std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const = 0;

  /// @return a clone of the attribute
  virtual std::unique_ptr<Attribute> forceClone(util::CloneVisitor &cv) const {
    return clone(cv);
  }

private:
  virtual std::ostream &doFormat(std::ostream &os) const = 0;
};

/// Attribute containing SrcInfo
struct SrcInfoAttribute : public Attribute {
  static const std::string AttributeName;

  /// source info
  codon::SrcInfo info;

  SrcInfoAttribute() = default;
  /// Constructs a SrcInfoAttribute.
  /// @param info the source info
  explicit SrcInfoAttribute(codon::SrcInfo info) : info(std::move(info)) {}

  std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const override {
    return std::make_unique<SrcInfoAttribute>(*this);
  }

private:
  std::ostream &doFormat(std::ostream &os) const override { return os << info; }
};

/// Attribute containing docstring from source
struct DocstringAttribute : public Attribute {
  static const std::string AttributeName;

  /// the docstring
  std::string docstring;

  DocstringAttribute() = default;
  /// Constructs a DocstringAttribute.
  /// @param docstring the docstring
  explicit DocstringAttribute(const std::string &docstring) : docstring(docstring) {}

  std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const override {
    return std::make_unique<DocstringAttribute>(*this);
  }

private:
  std::ostream &doFormat(std::ostream &os) const override { return os << docstring; }
};

/// Attribute containing function information
struct KeyValueAttribute : public Attribute {
  static const std::string AttributeName;

  /// attributes map
  std::map<std::string, std::string> attributes;

  KeyValueAttribute() = default;
  /// Constructs a KeyValueAttribute.
  /// @param attributes the map of attributes
  explicit KeyValueAttribute(std::map<std::string, std::string> attributes)
      : attributes(std::move(attributes)) {}

  /// @param key the key
  /// @return true if the map contains key, false otherwise
  bool has(const std::string &key) const;

  /// @param key the key
  /// @return the value associated with the given key, or empty
  ///         string if none
  std::string get(const std::string &key) const;

  std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const override {
    return std::make_unique<KeyValueAttribute>(*this);
  }

private:
  std::ostream &doFormat(std::ostream &os) const override;
};

/// Attribute containing type member information
struct MemberAttribute : public Attribute {
  static const std::string AttributeName;

  /// member source info map
  std::map<std::string, SrcInfo> memberSrcInfo;

  MemberAttribute() = default;
  /// Constructs a KeyValueAttribute.
  /// @param attributes the map of attributes
  explicit MemberAttribute(std::map<std::string, SrcInfo> memberSrcInfo)
      : memberSrcInfo(std::move(memberSrcInfo)) {}

  std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const override {
    return std::make_unique<MemberAttribute>(*this);
  }

private:
  std::ostream &doFormat(std::ostream &os) const override;
};

/// Attribute attached to IR structures corresponding to tuple literals
struct TupleLiteralAttribute : public Attribute {
  static const std::string AttributeName;

  /// values contained in tuple literal
  std::vector<Value *> elements;

  explicit TupleLiteralAttribute(std::vector<Value *> elements)
      : elements(std::move(elements)) {}

  std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const override;
  std::unique_ptr<Attribute> forceClone(util::CloneVisitor &cv) const override;

private:
  std::ostream &doFormat(std::ostream &os) const override;
};

/// Information about an element in a collection literal
struct LiteralElement {
  /// the element value
  Value *value;
  /// true if preceded by "*", as in "[*x]"
  bool star;
};

/// Attribute attached to IR structures corresponding to list literals
struct ListLiteralAttribute : public Attribute {
  static const std::string AttributeName;

  /// elements contained in list literal
  std::vector<LiteralElement> elements;

  explicit ListLiteralAttribute(std::vector<LiteralElement> elements)
      : elements(std::move(elements)) {}

  std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const override;
  std::unique_ptr<Attribute> forceClone(util::CloneVisitor &cv) const override;

private:
  std::ostream &doFormat(std::ostream &os) const override;
};

/// Attribute attached to IR structures corresponding to set literals
struct SetLiteralAttribute : public Attribute {
  static const std::string AttributeName;

  /// elements contained in set literal
  std::vector<LiteralElement> elements;

  explicit SetLiteralAttribute(std::vector<LiteralElement> elements)
      : elements(std::move(elements)) {}

  std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const override;
  std::unique_ptr<Attribute> forceClone(util::CloneVisitor &cv) const override;

private:
  std::ostream &doFormat(std::ostream &os) const override;
};

/// Attribute attached to IR structures corresponding to dict literals
struct DictLiteralAttribute : public Attribute {
  struct KeyValuePair {
    /// the key in the literal
    Value *key;
    /// the value in the literal, or null if key is being star-unpacked
    Value *value;
  };

  static const std::string AttributeName;

  /// keys and values contained in dict literal
  std::vector<KeyValuePair> elements;

  explicit DictLiteralAttribute(std::vector<KeyValuePair> elements)
      : elements(std::move(elements)) {}

  std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const override;
  std::unique_ptr<Attribute> forceClone(util::CloneVisitor &cv) const override;

private:
  std::ostream &doFormat(std::ostream &os) const override;
};

/// Attribute attached to IR structures corresponding to partial functions
struct PartialFunctionAttribute : public Attribute {
  static const std::string AttributeName;

  /// base name of the function being used in the partial
  std::string name;

  /// partial arguments, or null if none
  /// e.g. "f(a, ..., b)" has elements [a, null, b]
  std::vector<Value *> args;

  PartialFunctionAttribute(const std::string &name, std::vector<Value *> args)
      : name(name), args(std::move(args)) {}

  std::unique_ptr<Attribute> clone(util::CloneVisitor &cv) const override;
  std::unique_ptr<Attribute> forceClone(util::CloneVisitor &cv) const override;

private:
  std::ostream &doFormat(std::ostream &os) const override;
};

} // namespace ir
} // namespace codon

template <> struct fmt::formatter<codon::ir::Attribute> : fmt::ostream_formatter {};
