#include "vulkan/pipeline.hpp"
#include "vulkan/vulkan_utils.hpp"

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
    "Failed to create shader module");

  return shaderModule;
}

VkVertexInputBindingDescription getDefaultVertexBindingDescription()
{
  VkVertexInputBindingDescription binding{};

  binding.binding = 0;
  binding.stride = sizeof(Vertex);
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return binding;
}

std::vector<VkVertexInputAttributeDescription> getDefaultAttributeDescriptions()
{
  std::vector<VkVertexInputAttributeDescription> attributes;

  {
    VkVertexInputAttributeDescription attr;
    attr.binding = 0;
    attr.location = 0;
    attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    attr.offset = offsetof(Vertex, pos);
    attributes.push_back(attr);
  }

  {
    VkVertexInputAttributeDescription attr;
    attr.binding = 0;
    attr.location = 1;
    attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    attr.offset = offsetof(Vertex, normal);
    attributes.push_back(attr);
  }

  {
    VkVertexInputAttributeDescription attr;
    attr.binding = 0;
    attr.location = 2;
    attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    attr.offset = offsetof(Vertex, colour);
    attributes.push_back(attr);
  }

  {
    VkVertexInputAttributeDescription attr;
    attr.binding = 0;
    attr.location = 3;
    attr.format = VK_FORMAT_R32G32_SFLOAT;
    attr.offset = offsetof(Vertex, texCoord);
    attributes.push_back(attr);
  }

  return attributes;
}
