#include "vulkan/vulkan_utils.hpp"

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
  VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
    if (typeFilter & (1 << i) &&
      (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {

      return i;
    }
  }

  EXCEPTION("Failed to find suitable memory type");
}

void createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height,
  VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
  VkImage& image, VkDeviceMemory& imageMemory, uint32_t arrayLayers, VkImageCreateFlags flags)
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = arrayLayers;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = flags;

  VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &image), "Failed to create image");

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits,
    properties);

  VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory),
    "Failed to allocate image memory");

  vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format,
  VkImageAspectFlags aspectFlags, VkImageViewType type, uint32_t layerCount)
{
  VkImageViewCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = image;
  createInfo.viewType = type;
  createInfo.format = format;
  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.subresourceRange.aspectMask = aspectFlags;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = layerCount;

  VkImageView imageView;

  VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &imageView),
    "Failed to create image view");

  return imageView;
}
