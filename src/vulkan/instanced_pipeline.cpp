#include "vulkan/instanced_pipeline.hpp"
#include "vulkan/vulkan_utils.hpp"
#include "utils.hpp"
#include "model.hpp"

namespace
{

VkVertexInputBindingDescription getInstanceBindingDescription()
{
  VkVertexInputBindingDescription binding{};

  binding.binding = 1;
  binding.stride = sizeof(MeshInstance);
  binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

  return binding;
}

std::vector<VkVertexInputAttributeDescription> getInstancedAttributeDescriptions()
{
  auto attributes = defaultAttributeDescriptions();
  size_t n = attributes.size();

  for (unsigned int i = 0; i < 4; ++i) {
    uint32_t offset = offsetof(MeshInstance, modelMatrix) + 4 * sizeof(float_t) * i;

    VkVertexInputAttributeDescription attr;
    attr.location = n + i;
    attr.binding = 1;
    attr.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attr.offset = offset;

    attributes.push_back(attr);
  }

  return attributes;
}

} // namespace

InstancedPipeline::InstancedPipeline(VkDevice device, VkExtent2D swapchainExtent,
  VkRenderPass renderPass, const RenderResources& renderResources)
  : m_device(device)
  , m_renderResources(renderResources)
{
  auto vertShaderCode = readBinaryFile("shaders/vertex/instanced.spv");
  auto fragShaderCode = readBinaryFile("shaders/fragment/default.spv");

  VkShaderModule vertShaderModule = createShaderModule(m_device, vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(m_device, fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  auto vertexBindingDescription = defaultVertexBindingDescription();
  auto instanceBindingDescription = getInstanceBindingDescription();
  auto attributeDescriptions = getInstancedAttributeDescriptions();

  std::vector<VkVertexInputBindingDescription> bindingDescriptions{
    vertexBindingDescription,
    instanceBindingDescription
  };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
  vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
  vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  auto inputAssembly = defaultInputAssemblyState();
  VkViewport viewport;
  VkRect2D scissor;
  auto viewportState = defaultViewportState(viewport, scissor, swapchainExtent);
  auto rasterizer = defaultRasterizationState();
  auto multisampling = defaultMultisamplingState();
  VkPipelineColorBlendAttachmentState colourBlendAttachment;
  auto colourBlending = defaultColourBlendState(colourBlendAttachment);

  VkPushConstantRange fragmentPushConstants;
  fragmentPushConstants.offset = 64;
  fragmentPushConstants.size = sizeof(VkBool32);
  fragmentPushConstants.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkPushConstantRange, 1> pushConstantRanges{
    fragmentPushConstants
  };

  std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts{
    m_renderResources.getUboDescriptorSetLayout(),
    m_renderResources.getMaterialDescriptorSetLayout()
  };

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
  pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
  VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_layout),
    "Failed to create instanced pipeline layout");

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  auto depthStencil = defaultDepthStencilState();

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colourBlending;
  pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.layout = m_layout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
    &m_pipeline), "Failed to create instanced pipeline");

  vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
  vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

void InstancedPipeline::recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node_,
  size_t currentFrame)
{
  auto& node = dynamic_cast<const InstancedModelNode&>(node_);
  VkBool32 bUseMaterial = node.material != NULL_ID;

  auto uboDescriptorSet = m_renderResources.getUboDescriptorSet(currentFrame);
  auto materialDescriptorSet = m_renderResources.getMaterialDescriptorSet(node.material);
  auto buffers = m_renderResources.getMeshBuffers(node.mesh);

  // TODO: Only bind things that have changed since last iteration
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
  std::vector<VkBuffer> vertexBuffers{ buffers.vertexBuffer, buffers.instanceBuffer };
  std::vector<VkDeviceSize> offsets(vertexBuffers.size(), 0);
  vkCmdBindVertexBuffers(commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(),
    offsets.data());
  vkCmdBindIndexBuffer(commandBuffer, buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 0, 1,
    &uboDescriptorSet, 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    m_layout, 1, 1, &materialDescriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer, m_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(VkBool32),
    &bUseMaterial);
  vkCmdDrawIndexed(commandBuffer, buffers.numIndices, buffers.numInstances, 0, 0, 0);
}

InstancedPipeline::~InstancedPipeline()
{
  vkDestroyPipeline(m_device, m_pipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_layout, nullptr);
}
