#pragma once

#include "vulkan/pipeline.hpp"

struct DefaultModelNode : public RenderNode
{
  DefaultModelNode()
    : RenderNode(RenderNodeType::defaultModel) {}

  ModelId model;
  Mat4x4f modelMatrix;
};

class DefaultPipeline
{
  public:
    DefaultPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass,
      VkDescriptorSetLayout uboDescriptorSetLayout,
      VkDescriptorSetLayout materialDescriptorSetLayout);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const ModelData& model,
      const DefaultModelNode& node, VkDescriptorSet uboDescriptorSet,
      VkDescriptorSet textureDescriptorSet);

    ~DefaultPipeline();

  private:
    VkDevice m_device;
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
};

using DefaultPipelinePtr = std::unique_ptr<DefaultPipeline>;
