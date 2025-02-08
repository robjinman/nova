#pragma once

#include "vulkan/pipeline.hpp"

struct SkyboxNode : public RenderNode
{
  SkyboxNode()
    : RenderNode(RenderNodeType::skybox) {}

  RenderItemId mesh;
  RenderItemId cubeMap;
};

class SkyboxPipeline
{
  public:
    SkyboxPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass,
      VkDescriptorSetLayout uboDescriptorSetLayout,
      VkDescriptorSetLayout cubeMapDescriptorSetLayout);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const MeshData& mesh,
      VkDescriptorSet uboDescriptorSet, VkDescriptorSet cubeMapDescriptorSet);

    ~SkyboxPipeline();

  private:
    VkDevice m_device;
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
};

using SkyboxPipelinePtr = std::unique_ptr<SkyboxPipeline>;
