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

VkVertexInputBindingDescription defaultVertexBindingDescription()
{
  VkVertexInputBindingDescription binding{};

  binding.binding = 0;
  binding.stride = sizeof(Vertex);
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return binding;
}

std::vector<VkVertexInputAttributeDescription> defaultAttributeDescriptions()
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

VkPipelineInputAssemblyStateCreateInfo defaultInputAssemblyState()
{
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  return inputAssembly;
}

VkPipelineViewportStateCreateInfo defaultViewportState(VkViewport& viewport, VkRect2D& scissor,
  VkExtent2D swapchainExtent)
{
  viewport = VkViewport{};
  viewport.x = 0.f;
  viewport.y = 0.f;
  viewport.width = swapchainExtent.width;
  viewport.height = swapchainExtent.height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;

  scissor = VkRect2D{};
  scissor.offset = { 0, 0 };
  scissor.extent = swapchainExtent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  return viewportState;
}

VkPipelineRasterizationStateCreateInfo defaultRasterizationState()
{
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  return rasterizer;
}

VkPipelineMultisampleStateCreateInfo defaultMultisamplingState()
{
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  return multisampling;
}

VkPipelineColorBlendStateCreateInfo
defaultColourBlendState(VkPipelineColorBlendAttachmentState& colourBlendAttachment)
{
  colourBlendAttachment = VkPipelineColorBlendAttachmentState{};
  colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                         VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT |
                                         VK_COLOR_COMPONENT_A_BIT;
  colourBlendAttachment.blendEnable = VK_FALSE;
  colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colourBlending{};
  colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colourBlending.logicOpEnable = VK_FALSE;
  colourBlending.logicOp = VK_LOGIC_OP_COPY;
  colourBlending.attachmentCount = 1;
  colourBlending.pAttachments = &colourBlendAttachment;
  colourBlending.blendConstants[0] = 0.0f;
  colourBlending.blendConstants[1] = 0.0f;
  colourBlending.blendConstants[2] = 0.0f;
  colourBlending.blendConstants[3] = 0.0f;

  return colourBlending;
}

VkPipelineDepthStencilStateCreateInfo defaultDepthStencilState()
{
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.f;
  depthStencil.maxDepthBounds = 1.f;
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {};
  depthStencil.back = {};

  return depthStencil;
}