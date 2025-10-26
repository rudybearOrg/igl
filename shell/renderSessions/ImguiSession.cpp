/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// @fb-only

#include <shell/renderSessions/ImguiSession.h>

#include <shell/shared/platform/DisplayContext.h>
#include <shell/shared/renderSession/ShellParams.h>
#include <igl/CommandBuffer.h>
#include <igl/RenderPass.h>

namespace igl::shell {

void ImguiSession::initialize() noexcept {
  commandQueue_ = getPlatform().getDevice().createCommandQueue({}, nullptr);

  // Create the ImGui session
  imguiSession_ = std::make_unique<iglu::imgui::Session>(getPlatform().getDevice(),
                                                         getPlatform().getInputDispatcher());
}

void ImguiSession::update(SurfaceTextures surfaceTextures) noexcept {
  const igl::DeviceScope deviceScope(getPlatform().getDevice());

  auto cmdBuffer = commandQueue_->createCommandBuffer({}, nullptr);

  const FramebufferDesc framebufferDesc = {
      .colorAttachments = {{.texture = surfaceTextures.color}},
  };
  if (outputFramebuffer_) {
    outputFramebuffer_->updateDrawable(surfaceTextures.color);
  } else {
    outputFramebuffer_ = getPlatform().getDevice().createFramebuffer(framebufferDesc, nullptr);
  }

  const RenderPassDesc renderPassDesc = {
      .colorAttachments = {{
          .loadAction = igl::LoadAction::Clear,
          .storeAction = igl::StoreAction::Store,
          .clearColor = {0.2f, 0.2f, 0.2f, 1.0f},  // Dark gray instead of yellow
      }},
  };
  auto encoder = cmdBuffer->createRenderCommandEncoder(renderPassDesc, outputFramebuffer_);

  { // Draw using ImGui every frame
    imguiSession_->beginFrame(framebufferDesc, getPlatform().getDisplayContext().pixelsPerPoint);
    ImGui::ShowDemoWindow();
    imguiSession_->endFrame(getPlatform().getDevice(), *encoder);
  }

  encoder->endEncoding();
  if (shellParams().shouldPresent) {
    cmdBuffer->present(surfaceTextures.color);
  }

  commandQueue_->submit(*cmdBuffer);
}

} // namespace igl::shell
