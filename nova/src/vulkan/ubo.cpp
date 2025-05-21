#include "vulkan/ubo.hpp"

namespace render
{

BufferedUbo::BufferedUbo(VkPhysicalDevice physicalDevice, VkDevice device, size_t size)
  : m_device(device)
{
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    auto& resources = m_resources[i];

    VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = size,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr
    };

    VK_CHECK(vkCreateBuffer(m_device, &bufferInfo, nullptr, &resources.buffer),
      "Failed to create buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, resources.buffer, &memRequirements);
    m_allocatedSize = memRequirements.size;

    VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = m_allocatedSize,
      .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    };

    VK_CHECK(vkAllocateMemory(m_device, &allocInfo, nullptr, &resources.memory),
      "Failed to allocate memory for buffer");

    vkBindBufferMemory(m_device, resources.buffer, resources.memory, 0);

    vkMapMemory(m_device, resources.memory, 0, m_allocatedSize, 0, &resources.mapped);
  }
}

void BufferedUbo::write(size_t frame, const void* data, size_t size)
{
  memcpy(m_resources[frame].mapped, data, size);

  VkMappedMemoryRange range{
    .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
    .pNext = nullptr,
    .memory = m_resources[frame].memory,
    .offset = 0,
    .size = m_allocatedSize
  };

  VK_CHECK(vkFlushMappedMemoryRanges(m_device, 1, &range), "Failed to flush memory ranges");
}

VkBuffer BufferedUbo::buffer(size_t frame) const
{
  return m_resources[frame].buffer;
}

BufferedUbo::~BufferedUbo()
{
  for (size_t i = 0; i < m_resources.size(); ++i) {
    vkDestroyBuffer(m_device, m_resources[i].buffer, nullptr);
    vkFreeMemory(m_device, m_resources[i].memory, nullptr);
  }
}

}
