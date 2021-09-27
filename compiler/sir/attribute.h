#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "util/common.h"

#include "util/fmt/format.h"
#include "util/fmt/ostream.h"

namespace seq {
namespace ir {

/// Base for SIR attributes.
struct Attribute {
  virtual ~Attribute() noexcept = default;

  /// @return true if the attribute should be propagated across clones
  virtual bool needsClone() const { return true; }

  friend std::ostream &operator<<(std::ostream &os, const Attribute &a) {
    return a.doFormat(os);
  }

  /// @return a clone of the attribute
  std::unique_ptr<Attribute> clone() const {
    return std::unique_ptr<Attribute>(doClone());
  }

private:
  virtual std::ostream &doFormat(std::ostream &os) const = 0;

  virtual Attribute *doClone() const = 0;
};

/// Attribute containing SrcInfo
struct SrcInfoAttribute : public Attribute {
  static const std::string AttributeName;

  /// source info
  seq::SrcInfo info;

  SrcInfoAttribute() = default;
  /// Constructs a SrcInfoAttribute.
  /// @param info the source info
  explicit SrcInfoAttribute(seq::SrcInfo info) : info(std::move(info)) {}

private:
  std::ostream &doFormat(std::ostream &os) const override { return os << info; }

  Attribute *doClone() const override { return new SrcInfoAttribute(*this); }
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

private:
  std::ostream &doFormat(std::ostream &os) const override;

  Attribute *doClone() const override { return new KeyValueAttribute(*this); }
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

private:
  std::ostream &doFormat(std::ostream &os) const override;

  Attribute *doClone() const override { return new MemberAttribute(*this); }
};

} // namespace ir
} // namespace seq
