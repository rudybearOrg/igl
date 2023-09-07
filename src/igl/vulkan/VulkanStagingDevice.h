/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <igl/vulkan/Common.h>
#include <igl/vulkan/VulkanHelpers.h>

namespace igl {
namespace vulkan {

class VulkanBuffer;
class VulkanContext;
class VulkanImage;
class VulkanImmediateCommands;

class VulkanStagingDevice final {
 public:
  explicit VulkanStagingDevice(VulkanContext& ctx);
  ~VulkanStagingDevice() = default;

  VulkanStagingDevice(const VulkanStagingDevice&) = delete;
  VulkanStagingDevice& operator=(const VulkanStagingDevice&) = delete;

  void bufferSubData(VulkanBuffer& buffer, size_t dstOffset, size_t size, const void* data);
  void getBufferSubData(VulkanBuffer& buffer, size_t srcOffset, size_t size, void* data);
  void imageData(VulkanImage& image,
                 TextureType type,
                 const TextureRangeDesc& range,
                 const TextureFormatProperties& properties,
                 uint32_t bytesPerRow,
                 const void* data);
  void getImageData2D(VkImage srcImage,
                      const uint32_t level,
                      const uint32_t layer,
                      const VkRect2D& imageRegion,
                      TextureFormatProperties properties,
                      VkFormat format,
                      VkImageLayout layout,
                      void* data,
                      uint32_t bytesPerRow,
                      bool flipImageVertical);

 private:
  struct MemoryRegionDesc {
    uint32_t srcOffset_ = 0;
    uint32_t alignedSize_ = 0;
  };

  uint32_t getAlignedSize(uint32_t size) const;
  MemoryRegionDesc getNextFreeOffset(uint32_t size);
  void flushOutstandingFences();

 private:
  VulkanContext& ctx_;
  std::shared_ptr<VulkanBuffer> stagingBuffer_;
  std::unique_ptr<VulkanImmediateCommands> immediate_;
  uint32_t stagingBufferFrontOffset_ = 0;
  uint32_t stagingBufferAlignment_ = 16; // updated to support BC7 compressed image
  uint32_t stagingBufferSize_;
  uint32_t bufferCapacity_;
  std::unordered_map<uint64_t, MemoryRegionDesc> outstandingFences_;
};

} // namespace vulkan
} // namespace igl
