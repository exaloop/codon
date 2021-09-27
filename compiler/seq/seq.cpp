#include "seq.h"
#include "pipeline.h"
#include "revcomp.h"

#include "sir/transform/lowering/pipeline.h"

namespace seq {

void Seq::addIRPasses(ir::transform::PassManager *pm, bool debug) {
  pm->registerPass(std::make_unique<KmerRevcompInterceptor>());
  if (debug)
    return;
  auto dep = ir::transform::lowering::PipelineLowering::KEY;
  pm->registerPass(std::make_unique<PipelineSubstitutionOptimization>(), dep);
  pm->registerPass(std::make_unique<PipelinePrefetchOptimization>(), dep);
  pm->registerPass(std::make_unique<PipelineInterAlignOptimization>(), dep);
}

} // namespace seq
