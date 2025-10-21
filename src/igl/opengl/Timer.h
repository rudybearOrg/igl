/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <igl/Timer.h>
#include <igl/opengl/GLIncludes.h>
#include <igl/opengl/IContext.h>
#include <igl/opengl/WithContext.h>

namespace igl::opengl {

class Timer : public WithContext, public ITimer {
 public:
  explicit Timer(IContext& context);
  ~Timer() override;

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;
  Timer(Timer&&) = delete;
  Timer& operator=(Timer&&) = delete;

  void end();

  [[nodiscard]] uint64_t getElapsedTimeNanos() const override;

  [[nodiscard]] bool resultsAvailable() const override;

 private:
  GLuint id_ = 0;
};

} // namespace igl::opengl
