#pragma once

#include "sir/dsl/nodes.h"
#include "sir/sir.h"
#include "sir/transform/pass.h"

namespace seq {

class KmerRevcomp : public ir::AcceptorExtend<KmerRevcomp, ir::dsl::CustomInstr> {
private:
  ir::Value *kmer;

public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

  explicit KmerRevcomp(ir::Value *kmer) : AcceptorExtend(), kmer(kmer) {}

  std::unique_ptr<ir::dsl::codegen::ValueBuilder> getBuilder() const override;
  std::unique_ptr<ir::dsl::codegen::CFBuilder> getCFBuilder() const override;

  bool match(const ir::Value *v) const override;
  ir::Value *doClone(ir::util::CloneVisitor &cv) const override;
  std::ostream &doFormat(std::ostream &os) const override;
};

class KmerRevcompInterceptor : public ir::transform::Pass {
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void run(ir::Module *) override;
};

} // namespace seq
