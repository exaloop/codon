// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <algorithm>
#include <atomic>
#include <vector>

#include "codon/cir/llvm/llvm.h"

namespace codon::ir::analyze::dataflow {

// Graphs are frozen. Workers only read CIR and write their own inspector;
// all mutation of the result map happens on the caller.
template <typename Inspector>
void analyzeGraphs(const std::vector<Inspector *> &jobs, unsigned threads) {
  if (jobs.size() < 64 || threads == 1 || !llvm::llvm_is_multithreaded()) {
    for (auto *job : jobs)
      job->analyze();
    return;
  }
  constexpr size_t chunkSize = 8;
  const size_t chunks = 1 + (jobs.size() - 1) / chunkSize;
  auto strategy = llvm::heavyweight_hardware_concurrency(threads);
  strategy.Limit = true;
  std::atomic<size_t> next{0};
  llvm::DefaultThreadPool pool(strategy);
  std::vector<std::shared_future<void>> workers;
  const auto workersNeeded = std::min<size_t>(pool.getMaxConcurrency(), chunks);
  for (size_t i = 0; i < workersNeeded; ++i)
    workers.push_back(pool.async([&] {
      for (;;) {
        auto start = next.fetch_add(chunkSize, std::memory_order_relaxed);
        if (start >= jobs.size())
          break;
        for (auto j = start; j < std::min(start + chunkSize, jobs.size()); ++j)
          jobs[j]->analyze();
      }
    }));
  pool.wait();
  for (auto &worker : workers)
    worker.get();
}

} // namespace codon::ir::analyze::dataflow
