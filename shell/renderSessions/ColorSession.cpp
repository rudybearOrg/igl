/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// @fb-only

#include <cstring>

#include <glm/gtc/color_space.hpp>
#include <shell/renderSessions/ColorSession.h>
#include <shell/shared/imageLoader/ImageLoader.h>
#include <shell/shared/platform/DisplayContext.h>
#include <shell/shared/renderSession/RenderSession.h>
#include <shell/shared/renderSession/ShellParams.h>
#include <igl/NameHandle.h>
#include <igl/ShaderCreator.h>
// @fb-only
// @fb-only
// @fb-only

namespace igl::shell {

namespace {

struct VertexPosUv {
  iglu::simdtypes::float3 position;
  iglu::simdtypes::float2 uv;
};
VertexPosUv vertexData[] = {
    {{-1.f, 1.f, 0.0}, {0.0, 0.0}},
    {{1.f, 1.f, 0.0}, {1.0, 0.0}},
    {{-1.f, -1.f, 0.0}, {0.0, 1.0}},
    {{1.f, -1.f, 0.0}, {1.0, 1.0}},
};
uint16_t indexData[] = {0, 1, 2, 1, 3, 2};

namespace {
// @fb-only
// @fb-only
  // @fb-only
          // @fb-only
          // @fb-only
          // @fb-only
          // @fb-only
          // @fb-only
// @fb-only
// @fb-only

BufferDesc getVertexBufferDesc(const igl::IDevice& device) {
// @fb-only
  // @fb-only
    // @fb-only
                                    // @fb-only
                                    // @fb-only
                                    // @fb-only
    // @fb-only
        // @fb-only
        // @fb-only
        // @fb-only
        // @fb-only
  // @fb-only
// @fb-only
  return {BufferDesc::BufferTypeBits::Vertex, vertexData, sizeof(vertexData)};
}

uint32_t getVertexBufferIndex(const igl::IDevice& device) {
// @fb-only
  // @fb-only
    return 0;
  // @fb-only
// @fb-only
  return 1;
}

ResourceStorage getIndexBufferResourceStorage(const igl::IDevice& device) {
// @fb-only
  // @fb-only
    // @fb-only
  // @fb-only
// @fb-only
  return igl::ResourceStorage::Invalid;
}
} // namespace

std::string getVersion() {
  return "#version 100";
}

std::string getMetalShaderSource() {
  return R"(
              using namespace metal;

              typedef struct {
                 float3 color;
                 float4x4 mvp;
               } UniformBlock;

              typedef struct {
                float3 position [[attribute(0)]];
                float2 uv [[attribute(1)]];
              } VertexIn;

              typedef struct {
                float4 position [[position]];
                float2 uv;
              } VertexOut;

              vertex VertexOut vertexShader(
                  uint vid [[vertex_id]], constant VertexIn * vertices [[buffer(1)]],
                  constant UniformBlock * ub [[buffer(0)]]) {
                VertexOut out;
                out.position = ub->mvp * float4(vertices[vid].position, 1.0);
                out.uv = vertices[vid].uv;
                return out;
              }

              fragment float4 fragmentShader(
                  VertexOut IN [[stage_in]],
                  texture2d<float> diffuseTex [[texture(0)]],
                  sampler linearSampler [[sampler(0)]],
                  constant UniformBlock * ub [[buffer(0)]]) {
                float4 tex = diffuseTex.sample(linearSampler, IN.uv);
                return float4(ub->color.r, ub->color.g, ub->color.b, 1.0) *
                      tex;
              }
    )";
}

std::string getMetalShaderSourceGradient() {
  return R"(
              using namespace metal;

              typedef struct {
                 float3 color;
                 float4x4 mvp;
               } UniformBlock;

              typedef struct {
                float3 position [[attribute(0)]];
                float2 uv [[attribute(1)]];
              } VertexIn;

              typedef struct {
                float4 position [[position]];
                float2 uv;
              } VertexOut;

              vertex VertexOut vertexShader(
                  uint vid [[vertex_id]], constant VertexIn * vertices [[buffer(1)]]) {
                VertexOut out;
                out.position = float4(vertices[vid].position, 1.0);
                out.uv = vertices[vid].uv;
                return out;
              }

              fragment float4 fragmentShader(
                  VertexOut IN [[stage_in]],
                  texture2d<float> diffuseTex [[texture(0)]],
                  sampler linearSampler [[sampler(0)]],
                  constant UniformBlock * color [[buffer(0)]]) {

                  float numSteps = 20.0;
                  float uvX;
                  if (IN.uv.y<0.25) {
                   uvX = IN.uv.x;
                  } else if (IN.uv.y<0.5) {
                    uvX = floor(IN.uv.x*numSteps+0.5)/numSteps;
                  } else if (IN.uv.y<0.75) {
                    uvX = 1.0-IN.uv.x;
                  } else {
                    uvX = floor((1.0-IN.uv.x)*numSteps+0.5)/numSteps;
                  }
                  return float4(uvX, uvX, uvX, 1.0); 
              }
    )";
}

std::string getOpenGLVertexShaderSource() {
  return getVersion() + R"(
                precision highp float;
                attribute vec3 position;
                attribute vec2 uv_in;

                uniform vec3 color;
                uniform mat4 mvp;
                uniform sampler2D inputImage;

                varying vec3 vColor;
                varying vec2 uv;

                void main() {
                  gl_Position = mvp * vec4(position, 1.0);
                  uv = uv_in; // position.xy * 0.5 + 0.5;
                  vColor = color;
                })";
}

std::string getOpenGLFragmentShaderSource() {
  return getVersion() + std::string(R"(
                precision highp float;
                uniform vec3 color;
                uniform mat4 mvp;
                uniform sampler2D inputImage;
                varying vec3 vColor;
                varying vec2 uv;

                void main() {
                  gl_FragColor =
                      vec4(vColor, 1.0) * texture2D(inputImage, uv);
                })");
}

std::string getOpenGLFragmentShaderSourceGradient() {
  return getVersion() + R"(    
                precision highp float;
                uniform vec3 color;
                uniform mat4 mvp;
                uniform sampler2D inputImage;
                varying vec3 vColor;
                varying vec2 uv;
                
                void main() {
                  float numSteps = 20.0;
                  float uvX;
                  if (uv.y<0.25) {
                    uvX = uv.x;
                  } else if (uv.y<0.5) {
                   uvX = floor(uv.x*numSteps+0.5)/numSteps;
                  } else if (uv.y<0.75) {
                   uvX = 1.0-uv.x;
                  } else {
                   uvX = floor((1.0-uv.x)*numSteps+0.5)/numSteps;
                  } 
                  gl_FragColor = vec4(vec3(uvX), 1.0);
                }
                )";
}

std::string getVulkanVertexShaderSource() {
  return R"(precision highp float;
            layout(location = 0) in vec3 position;
            layout(location = 1) in vec2 uv_in;
            layout(location = 0) out vec2 uv;
            layout(location = 1) out vec3 color;

            layout (set = 1, binding = 0, std140) uniform UniformsPerObject {
              vec3 color;
              mat4 mvp;
            } perObject;

            void main() {
              gl_Position = perObject.mvp * vec4(position, 1.0);
              uv = uv_in;
              color = perObject.color;
            }
            )";
}

std::string getVulkanFragmentShaderSource() {
  return R"(
                layout(location = 0) in vec2 uv;
                layout(location = 1) in vec3 color;
                layout(location = 0) out vec4 out_FragColor;

                layout(set = 0, binding = 0) uniform sampler2D in_texture;

                void main() {
                  out_FragColor = vec4(color, 1.0) * texture(in_texture, uv);
                }
                )";
}

std::string getVulkanFragmentShaderSourceGradient() {
  return R"(
                layout(location = 0) in vec2 uv;
                layout(location = 1) in vec3 color;
                layout(location = 0) out vec4 out_FragColor;

                void main() {
                  float numSteps = 20.0;
                  float uvX;
                  if (uv.y<0.25) {
                    uvX = uv.x;
                  } else if (uv.y<0.5) {
                   uvX = floor(uv.x*numSteps+0.5)/numSteps;
                  } else if (uv.y<0.75) {
                   uvX = 1.0-uv.x;
                  } else {
                   uvX = floor((1.0-uv.x)*numSteps+0.5)/numSteps;
                  } 
                  out_FragColor = vec4(vec3(uvX), 1.0);
                }
                )";
}

// @fb-only

} // namespace

std::unique_ptr<IShaderStages> ColorSession::getShaderStagesForBackend(IDevice& device) {
  switch (device.getBackendType()) {
  case igl::BackendType::Invalid:
  case igl::BackendType::Custom:
    IGL_DEBUG_ASSERT_NOT_REACHED();
    return nullptr;
  case igl::BackendType::Vulkan: {
    auto vertexSource = getVulkanVertexShaderSource();
    if (device.hasFeature(DeviceFeatures::Multiview)) {
      vertexSource = R"(#version 450
                        #extension GL_OVR_multiview2 : require
                        layout(num_views = 2) in;)" +
                     vertexSource;
    }
    return igl::ShaderStagesCreator::fromModuleStringInput(
        device,
        vertexSource.c_str(),
        "main",
        "",
        colorTestModes_ == ColorTestModes::Gradient
            ? getVulkanFragmentShaderSourceGradient().c_str()
            : getVulkanFragmentShaderSource().c_str(),
        "main",
        "",
        nullptr);
  }
  // @fb-only
    // @fb-only
        // @fb-only
        // @fb-only
        // @fb-only
        // @fb-only
        // @fb-only
            // @fb-only
            // @fb-only
        // @fb-only
        // @fb-only
        // @fb-only
  case igl::BackendType::Metal:
    return igl::ShaderStagesCreator::fromLibraryStringInput(
        device,
        colorTestModes_ == ColorTestModes::Gradient ? getMetalShaderSourceGradient().c_str()
                                                    : getMetalShaderSource().c_str(),
        "vertexShader",
        "fragmentShader",
        "",
        nullptr);
  case igl::BackendType::OpenGL:
    return igl::ShaderStagesCreator::fromModuleStringInput(
        device,
        getOpenGLVertexShaderSource().c_str(),
        "main",
        "",
        colorTestModes_ == ColorTestModes::Gradient
            ? getOpenGLFragmentShaderSourceGradient().c_str()
            : getOpenGLFragmentShaderSource().c_str(),
        "main",
        "",
        nullptr);
  }
  IGL_UNREACHABLE_RETURN(nullptr)
}

void ColorSession::initialize() noexcept {
  glm::dvec3 linearOrangeColor = glm::dvec3{1.0, 0.5, 0.0};
  if (swapchainColorTextureformat_ == igl::TextureFormat::RGBA_SRGB &&
      getPlatform().getDevice().hasFeature(DeviceFeatures::SRGB)) {
    linearOrangeColor = glm::convertSRGBToLinear(linearOrangeColor);
  }
  glm::vec3 fLinearOrangeColor = glm::vec3(linearOrangeColor);
  iglu::simdtypes::float3 gpuLinearOrangeColor = {
      fLinearOrangeColor.x, fLinearOrangeColor.y, fLinearOrangeColor.z};

  auto& device = getPlatform().getDevice();

  // Vertex & Index buffer
  const BufferDesc vbDesc = getVertexBufferDesc(device);
  vb0_ = device.createBuffer(vbDesc, nullptr);
  IGL_DEBUG_ASSERT(vb0_ != nullptr);
  const BufferDesc ibDesc = BufferDesc(BufferDesc::BufferTypeBits::Index,
                                       indexData,
                                       sizeof(indexData),
                                       getIndexBufferResourceStorage(device));
  ib0_ = device.createBuffer(ibDesc, nullptr);
  IGL_DEBUG_ASSERT(ib0_ != nullptr);

  auto vertexBufferIndex = getVertexBufferIndex(getPlatform().getDevice());
  VertexInputStateDesc inputDesc;
  inputDesc.numAttributes = 2;
  inputDesc.attributes[0] = VertexAttribute{.bufferIndex = vertexBufferIndex,
                                            .format = VertexAttributeFormat::Float3,
                                            .offset = offsetof(VertexPosUv, position),
                                            .name = "position",
                                            .location = 0};
  inputDesc.attributes[1] = VertexAttribute{
      vertexBufferIndex, VertexAttributeFormat::Float2, offsetof(VertexPosUv, uv), "uv_in", 1};
  inputDesc.numInputBindings = 1;
  inputDesc.inputBindings[vertexBufferIndex].stride = sizeof(VertexPosUv);
  vertexInput0_ = device.createVertexInputState(inputDesc, nullptr);
  IGL_DEBUG_ASSERT(vertexInput0_ != nullptr);

  // Sampler & Texture
  SamplerStateDesc samplerDesc;
  samplerDesc.minFilter = samplerDesc.magFilter = SamplerMinMagFilter::Linear;
  samplerDesc.debugName = "Sampler: linear";
  samp0_ = device.createSamplerState(samplerDesc, nullptr);
  IGL_DEBUG_ASSERT(samp0_ != nullptr);

  if (colorTestModes_ == ColorTestModes::MacbethTexture) {
    tex0_ = getPlatform().loadTexture("macbeth.png", true, swapchainColorTextureformat_);
  } else if (colorTestModes_ == ColorTestModes::MacbethTextureKtx2) {
    tex0_ = getPlatform().loadTexture("macbeth.ktx2", true, swapchainColorTextureformat_);
  } else if (colorTestModes_ == ColorTestModes::OrangeTexture) {
    tex0_ = getPlatform().loadTexture("orange.png", true, swapchainColorTextureformat_);
  } else if (colorTestModes_ == ColorTestModes::OrangeClear) {
    tex0_ = getPlatform().loadTexture(igl::shell::ImageLoader::white());
    setPreferredClearColor(
        Color{fLinearOrangeColor.x, fLinearOrangeColor.y, fLinearOrangeColor.z, 1.0f});
  } else if (colorTestModes_ == ColorTestModes::Gradient) {
    tex0_ = getPlatform().loadTexture(igl::shell::ImageLoader::white());
  }
  shaderStages_ = getShaderStagesForBackend(device);
  IGL_DEBUG_ASSERT(shaderStages_ != nullptr);

  // Command queue
  const CommandQueueDesc desc{};
  commandQueue_ = device.createCommandQueue(desc, nullptr);
  IGL_DEBUG_ASSERT(commandQueue_ != nullptr);

  renderPass_.colorAttachments.resize(1);
  renderPass_.colorAttachments[0].loadAction = LoadAction::Clear;
  renderPass_.colorAttachments[0].storeAction = StoreAction::Store;
  renderPass_.colorAttachments[0].clearColor = getPreferredClearColor();
  renderPass_.depthAttachment.loadAction = LoadAction::Clear;
  renderPass_.depthAttachment.clearDepth = 1.0;

  // init uniforms
  glm::mat4x4 mvp(1.0f);
  memcpy(&fragmentParameters_.mvp, &mvp, sizeof(mvp));
  fragmentParameters_.color = (colorTestModes_ == ColorTestModes::OrangeClear)
                                  ? gpuLinearOrangeColor
                                  : iglu::simdtypes::float3{1.0f, 1.0f, 1.0f};

  BufferDesc fpDesc;
  fpDesc.type = BufferDesc::BufferTypeBits::Uniform;
  fpDesc.data = &fragmentParameters_;
  fpDesc.length = sizeof(fragmentParameters_);
  fpDesc.storage = ResourceStorage::Shared;

  fragmentParamBuffer_ = device.createBuffer(fpDesc, nullptr);
  IGL_DEBUG_ASSERT(fragmentParamBuffer_ != nullptr);
}

void ColorSession::update(SurfaceTextures surfaceTextures) noexcept {
  Result ret;
  if (framebuffer_ == nullptr) {
    FramebufferDesc framebufferDesc;
    framebufferDesc.colorAttachments[0].texture = surfaceTextures.color;
    framebufferDesc.depthAttachment.texture = surfaceTextures.depth;
    framebufferDesc.mode = surfaceTextures.color->getNumLayers() > 1 ? FramebufferMode::Stereo
                                                                     : FramebufferMode::Mono;
    IGL_DEBUG_ASSERT(ret.isOk());
    framebuffer_ = getPlatform().getDevice().createFramebuffer(framebufferDesc, &ret);
    IGL_DEBUG_ASSERT(ret.isOk());
    IGL_DEBUG_ASSERT(framebuffer_ != nullptr);
  } else {
    framebuffer_->updateDrawable(surfaceTextures.color);
  }

  const size_t textureUnit = 0;

  // Graphics pipeline
  if (pipelineState_ == nullptr) {
    RenderPipelineDesc graphicsDesc;
    graphicsDesc.vertexInputState = vertexInput0_;
    graphicsDesc.shaderStages = shaderStages_;
    graphicsDesc.targetDesc.colorAttachments.resize(1);
    graphicsDesc.targetDesc.colorAttachments[0].textureFormat =
        framebuffer_->getColorAttachment(0)->getProperties().format;
    graphicsDesc.targetDesc.depthAttachmentFormat =
        framebuffer_->getDepthAttachment()->getProperties().format;
    graphicsDesc.fragmentUnitSamplerMap[textureUnit] = IGL_NAMEHANDLE("inputImage");
    graphicsDesc.cullMode = igl::CullMode::Back;
    graphicsDesc.frontFaceWinding = igl::WindingMode::Clockwise;
    graphicsDesc.targetDesc.colorAttachments[0].blendEnabled = true;
    graphicsDesc.targetDesc.colorAttachments[0].rgbBlendOp = BlendOp::Add;
    graphicsDesc.targetDesc.colorAttachments[0].alphaBlendOp = BlendOp::Add;
    graphicsDesc.targetDesc.colorAttachments[0].srcRGBBlendFactor = BlendFactor::SrcAlpha;
    graphicsDesc.targetDesc.colorAttachments[0].srcAlphaBlendFactor = BlendFactor::SrcAlpha;
    graphicsDesc.targetDesc.colorAttachments[0].dstRGBBlendFactor = BlendFactor::OneMinusSrcAlpha;
    graphicsDesc.targetDesc.colorAttachments[0].dstAlphaBlendFactor = BlendFactor::OneMinusSrcAlpha;

    pipelineState_ = getPlatform().getDevice().createRenderPipeline(graphicsDesc, nullptr);
    IGL_DEBUG_ASSERT(pipelineState_ != nullptr);
  }

  // Command Buffers
  const CommandBufferDesc cbDesc;
  auto buffer = commandQueue_->createCommandBuffer(cbDesc, nullptr);
  IGL_DEBUG_ASSERT(buffer != nullptr);
  auto drawableSurface = framebuffer_->getColorAttachment(0);

  framebuffer_->updateDrawable(drawableSurface);

  // Uniform: "color"
  fragmentUniformDescriptors_.emplace_back();
  // @fb-only
    // @fb-only
    // @fb-only
    // @fb-only
  // @fb-only
    if (getPlatform().getDevice().hasFeature(DeviceFeatures::BindUniform)) {
      fragmentUniformDescriptors_.back().location =
          pipelineState_->getIndexByName("color", igl::ShaderStage::Fragment);
    }
  fragmentUniformDescriptors_.back().type = UniformType::Float3;
  fragmentUniformDescriptors_.back().offset = offsetof(FragmentFormat, color);

  // Uniform: "mvp"
  fragmentUniformDescriptors_.emplace_back();
  // @fb-only
    // @fb-only
    // @fb-only
    // @fb-only
  // @fb-only
    if (getPlatform().getDevice().hasFeature(DeviceFeatures::BindUniform)) {
      fragmentUniformDescriptors_.back().location =
          pipelineState_->getIndexByName("mvp", igl::ShaderStage::Fragment);
    }
  fragmentUniformDescriptors_.back().type = UniformType::Mat4x4;
  fragmentUniformDescriptors_.back().offset = offsetof(FragmentFormat, mvp);

  const auto& mvp = getPlatform().getDisplayContext().preRotationMatrix;
  memcpy(&fragmentParameters_.mvp, &mvp, sizeof(mvp));
  fragmentParamBuffer_->upload(&fragmentParameters_, {sizeof(fragmentParameters_)});

  // Submit commands
  const std::shared_ptr<IRenderCommandEncoder> commands =
      buffer->createRenderCommandEncoder(renderPass_, framebuffer_);
  IGL_DEBUG_ASSERT(commands != nullptr);
  if (commands) {
    commands->bindVertexBuffer(getVertexBufferIndex(getPlatform().getDevice()), *vb0_);
    commands->bindRenderPipelineState(pipelineState_);
    if (getPlatform().getDevice().hasFeature(DeviceFeatures::BindUniform)) {
      // Bind non block uniforms
      for (const auto& uniformDesc : fragmentUniformDescriptors_) {
        commands->bindUniform(uniformDesc, &fragmentParameters_);
      }
    } else if (getPlatform().getDevice().hasFeature(DeviceFeatures::UniformBlocks)) {
      // @fb-only
        // @fb-only
                            // @fb-only
                            // @fb-only
                            // @fb-only
      // @fb-only
        commands->bindBuffer(0, fragmentParamBuffer_.get());
      // @fb-only
    } else {
      IGL_DEBUG_ASSERT_NOT_REACHED();
    }
    // if (colorTestModes_ != ColorTestModes::eGradient) {
    commands->bindTexture(textureUnit, BindTarget::kFragment, tex0_.get());
    commands->bindSamplerState(textureUnit, BindTarget::kFragment, samp0_.get());
    //}
    commands->bindIndexBuffer(*ib0_, IndexFormat::UInt16);
    commands->drawIndexed(6);

    commands->endEncoding();
  }

  IGL_DEBUG_ASSERT(buffer != nullptr);
  if (shellParams().shouldPresent) {
    buffer->present(drawableSurface);
  }

  IGL_DEBUG_ASSERT(commandQueue_ != nullptr);
  commandQueue_->submit(*buffer, true);
  RenderSession::update(surfaceTextures);
}

} // namespace igl::shell
