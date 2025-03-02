#include "vulkan/default_pipeline.hpp"
#include "vulkan/vulkan_utils.hpp"
#include "vulkan/render_resources.hpp"
#include "platform_paths.hpp"
#include "utils.hpp"
#include "model.hpp"

DefaultPipeline::DefaultPipeline(const PlatformPaths& platformPaths, VkDevice device,
  VkExtent2D swapchainExtent, VkRenderPass renderPass, const RenderResources& renderResources)
  : m_device(device)
  , m_renderResources(renderResources)
{
  auto vertShaderCode = readBinaryFile(platformPaths.get("shaders", "vertex/default.spv"));
  auto fragShaderCode = readBinaryFile(platformPaths.get("shaders", "fragment/default.spv"));

  VkShaderModule vertShaderModule = createShaderModule(m_device, vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(m_device, fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertShaderModule,
    .pName = "main"
  };

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragShaderModule,
    .pName = "main"
  };

  auto vertexBindingDescription = defaultVertexBindingDescription();
  auto attributeDescriptions = defaultAttributeDescriptions();

  std::vector<VkVertexInputBindingDescription> bindingDescriptions{
    vertexBindingDescription
  };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size()),
    .pVertexBindingDescriptions = bindingDescriptions.data(),
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
    .pVertexAttributeDescriptions = attributeDescriptions.data()
  };

  auto inputAssembly = defaultInputAssemblyState();
  VkViewport viewport;
  VkRect2D scissor;
  auto viewportState = defaultViewportState(viewport, scissor, swapchainExtent);
  auto rasterizer = defaultRasterizationState();
  auto multisampling = defaultMultisamplingState();
  VkPipelineColorBlendAttachmentState colourBlendAttachment;
  auto colourBlending = defaultColourBlendState(colourBlendAttachment);

  VkPushConstantRange vertexPushConstants{
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .offset = 0,
    .size = sizeof(Mat4x4f)
  };

  std::vector<VkPushConstantRange> pushConstantRanges{
    vertexPushConstants
  };

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
    m_renderResources.getMatricesDescriptorSetLayout(),
    m_renderResources.getMaterialDescriptorSetLayout(),
    m_renderResources.getLightingDescriptorSetLayout()
  };

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
    .pSetLayouts = descriptorSetLayouts.data(),
    .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
    .pPushConstantRanges = pushConstantRanges.data()
  };

  VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_layout),
    "Failed to create default pipeline layout");

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  auto depthStencil = defaultDepthStencilState();

  VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = shaderStages,
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pDepthStencilState = &depthStencil,
    .pColorBlendState = &colourBlending,
    .pDynamicState = nullptr,
    .layout = m_layout,
    .renderPass = renderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };

  VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
    &m_pipeline), "Failed to create default pipeline");

  vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
  vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

void DefaultPipeline::recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node_,
  size_t currentFrame)
{
  auto& node = dynamic_cast<const DefaultModelNode&>(node_);

  auto matricesDescriptorSet = m_renderResources.getMatricesDescriptorSet(currentFrame);
  auto materialDescriptorSet = m_renderResources.getMaterialDescriptorSet(node.material);
  auto lightingDescriptorSet = m_renderResources.getLightingDescriptorSet(currentFrame);
  auto buffers = m_renderResources.getMeshBuffers(node.mesh);

  // TODO: Only bind things that have changed since last iteration
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
  std::vector<VkBuffer> vertexBuffers{ buffers.vertexBuffer };
  std::vector<VkDeviceSize> offsets(vertexBuffers.size(), 0);
  vkCmdBindVertexBuffers(commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(),
    offsets.data());
  vkCmdBindIndexBuffer(commandBuffer, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
  // TODO: Combine into single call
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 0, 1,
    &matricesDescriptorSet, 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 1, 1,
    &materialDescriptorSet, 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 2, 1,
    &lightingDescriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer, m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4x4f),
    &node.modelMatrix);
  vkCmdDrawIndexed(commandBuffer, buffers.numIndices, 1, 0, 0, 0);
}

DefaultPipeline::~DefaultPipeline()
{
  vkDestroyPipeline(m_device, m_pipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_layout, nullptr);
}
