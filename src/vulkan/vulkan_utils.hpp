#pragma once

#include "exception.hpp"
#include <vulkan/vulkan.h>

#define VK_CHECK(fnCall, msg) \
  { \
    VkResult MAC_code = fnCall; \
    if (MAC_code != VK_SUCCESS) { \
      EXCEPTION(msg << " (result: " << MAC_code << ")"); \
    } \
  }

const int MAX_FRAMES_IN_FLIGHT = 2;

void createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height,
  VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
  VkImage& image, VkDeviceMemory& imageMemory);

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format,
  VkImageAspectFlags aspectFlags);

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
  VkMemoryPropertyFlags properties);
