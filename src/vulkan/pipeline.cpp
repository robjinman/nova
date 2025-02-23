#include "vulkan/pipeline.hpp"
#include "vulkan/vulkan_utils.hpp"

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
{
  VkShaderModuleCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = code.size(),
    .pCode = reinterpret_cast<const uint32_t*>(code.data())
  };

  VkShaderModule shaderModule;
  VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
    "Failed to create shader module");

  return shaderModule;
}

VkVertexInputBindingDescription defaultVertexBindingDescription()
{
  return VkVertexInputBindingDescription{
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };
}

std::vector<VkVertexInputAttributeDescription> defaultAttributeDescriptions()
{
  std::vector<VkVertexInputAttributeDescription> attributes;

  attributes.push_back(VkVertexInputAttributeDescription{
    .binding = 0,
    .location = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(Vertex, pos)
  });

  attributes.push_back(VkVertexInputAttributeDescription{
    .binding = 0,
    .location = 1,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(Vertex, normal)
  });

  attributes.push_back(VkVertexInputAttributeDescription{
    .binding = 0,
    .location = 2,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(Vertex, colour)
  });

  attributes.push_back(VkVertexInputAttributeDescription{
    .binding = 0,
    .location = 3,
    .format = VK_FORMAT_R32G32_SFLOAT,
    .offset = offsetof(Vertex, texCoord)
  });

  return attributes;
}

VkPipelineInputAssemblyStateCreateInfo defaultInputAssemblyState()
{
  return VkPipelineInputAssemblyStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };
}

VkPipelineViewportStateCreateInfo defaultViewportState(VkViewport& viewport, VkRect2D& scissor,
  VkExtent2D swapchainExtent)
{
  viewport = VkViewport{
    .x = 0.f,
    .y = 0.f,
    .width = static_cast<float>(swapchainExtent.width),
    .height = static_cast<float>(swapchainExtent.height),
    .minDepth = 0.f,
    .maxDepth = 1.f
  };

  scissor = VkRect2D{
    .offset = { 0, 0 },
    .extent = swapchainExtent
  };

  return VkPipelineViewportStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor
  };
}

VkPipelineRasterizationStateCreateInfo defaultRasterizationState()
{
  return VkPipelineRasterizationStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f
  };
}

VkPipelineMultisampleStateCreateInfo defaultMultisamplingState()
{
  return VkPipelineMultisampleStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1.0f,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };
}

VkPipelineColorBlendStateCreateInfo
defaultColourBlendState(VkPipelineColorBlendAttachmentState& colourBlendAttachment)
{
  colourBlendAttachment = VkPipelineColorBlendAttachmentState{
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                      VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT |
                      VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
  };

  return VkPipelineColorBlendStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colourBlendAttachment,
    .blendConstants[0] = 0.0f,
    .blendConstants[1] = 0.0f,
    .blendConstants[2] = 0.0f,
    .blendConstants[3] = 0.0f
  };
}

VkPipelineDepthStencilStateCreateInfo defaultDepthStencilState()
{
  return VkPipelineDepthStencilStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .minDepthBounds = 0.f,
    .maxDepthBounds = 1.f,
    .stencilTestEnable = VK_FALSE,
    .front = {},
    .back = {}
  };
}
