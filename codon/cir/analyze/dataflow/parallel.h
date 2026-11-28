// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <algorithm>
#include <atomic>
#include <vector>

#include "codon/cir/llvm/llvm.h"

namespace codon::ir::analyze::dataflow {

unsigned getAnalysisThreads();

// Graphs are frozen. Workers only read CIR and write their own inspector;
// all mutation of the result map happens on the caller.
template <typename Inspector> void analyzeGraphs(const std::vector<Inspector *> &jobs) {
  const auto threads = getAnalysisThreads();
  if (jobs.size() < 64 || threads == 1 || !llvm::llvm_is_multithreaded()) {
    for (auto *job : jobs)
      job->analyze();
    return;
  }
  auto strategy = llvm::heavyweight_hardware_concurrency(threads);
  strategy.Limit = true;
  std::atomic<size_t> next{0};
  llvm::DefaultThreadPool pool(strategy);
  std::vector<std::shared_future<void>> workers;
  for (size_t i = 0; i < std::min<size_t>(pool.getMaxConcurrency(), jobs.size()); ++i)
    workers.push_back(pool.async([&] {
      for (;;) {
        auto start = next.fetch_add(8, std::memory_order_relaxed);
        if (start >= jobs.size())
          break;
        for (auto j = start; j < std::min(start + 8, jobs.size()); ++j)
          jobs[j]->analyze();
      }
    }));
  pool.wait();
  for (auto &worker : workers)
    worker.get();
}

} // namespace codon::ir::analyze::dataflow
