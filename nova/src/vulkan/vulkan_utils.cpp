#include "vulkan/vulkan_utils.hpp"
#include "utils.hpp"
#include <vector>

PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingFn;
PFN_vkCmdEndRenderingKHR vkCmdEndRenderingFn;

void loadVulkanExtensionFunctions(VkInstance instance)
{
  auto getFn = [&](const char* name) {
    auto ptr = vkGetInstanceProcAddr(instance, name);
    ASSERT(ptr != nullptr, STR("Error loading function " << name));
    return ptr;
  };

  vkCmdBeginRenderingFn =
    reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(getFn("vkCmdBeginRenderingKHR"));
  vkCmdEndRenderingFn =
    reinterpret_cast<PFN_vkCmdEndRenderingKHR>(getFn("vkCmdEndRenderingKHR"));
}

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
  VkImageCreateInfo imageInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = nullptr,
    .flags = flags,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = format,
    .extent = VkExtent3D{
      .width = width,
      .height = height,
      .depth = 1
    },
    .mipLevels = 1,
    .arrayLayers = arrayLayers,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = tiling,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
  };

  VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &image), "Failed to create image");

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = nullptr,
    .allocationSize = memRequirements.size,
    .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
  };

  VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory),
    "Failed to allocate image memory");

  vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format,
  VkImageAspectFlags aspectFlags, VkImageViewType type, uint32_t layerCount)
{
  VkImageViewCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .image = image,
    .viewType = type,
    .format = format,
    .components = VkComponentMapping{
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY
    },
    .subresourceRange = VkImageSubresourceRange{
      .aspectMask = aspectFlags,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = layerCount
    }
  };

  VkImageView imageView;

  VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &imageView),
    "Failed to create image view");

  return imageView;
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice)
{
  auto findSupportedFormat = [=](const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features) {

    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
        return format;
      }
      else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
        (props.optimalTilingFeatures & features) == features) {

        return format;
      }
    }

    EXCEPTION("Failed to find supported format");
  };

  return findSupportedFormat({
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT
  }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
