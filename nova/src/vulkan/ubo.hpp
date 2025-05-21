#pragma once

#include "vulkan/vulkan_utils.hpp"
#include <array>
#include <memory>

namespace render
{

struct UboResources
{
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  void* mapped = nullptr;
};

class BufferedUbo
{
  public:
    BufferedUbo(VkPhysicalDevice physicalDevice, VkDevice device, size_t size);

    void write(size_t frame, const void* data, size_t size);
    VkBuffer buffer(size_t frame) const;

    ~BufferedUbo();

  private:
    VkDevice m_device;
    std::array<UboResources, MAX_FRAMES_IN_FLIGHT> m_resources;
    VkDeviceSize m_allocatedSize = 0;
};

using BufferedUboPtr = std::unique_ptr<BufferedUbo>;

} // namespace render
