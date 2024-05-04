// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <vector>

namespace codon {
namespace ir {
namespace util {

/// Base for CIR visitor contexts.
template <typename Frame> class CIRContext {
private:
  std::vector<Frame> frames;

public:
  /// Emplaces a frame onto the stack.
  /// @param args a parameter pack of the arguments
  template <typename... Args> void emplaceFrame(Args... args) {
    frames.emplace_back(args...);
  }
  /// Replaces a frame.
  /// @param newFrame the new frame
  void replaceFrame(Frame newFrame) {
    frames.pop_back();
    frames.push_back(newFrame);
  }
  /// @return all frames
  std::vector<Frame> &getFrames() { return frames; }
  /// @return the current frame
  Frame &getFrame() { return frames.back(); }
  /// Pops a frame.
  void popFrame() { return frames.pop_back(); }
};

} // namespace util
} // namespace ir
} // namespace codon
