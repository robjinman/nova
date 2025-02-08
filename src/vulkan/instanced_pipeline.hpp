#pragma once

#include "vulkan/pipeline.hpp"

struct InstanceData
{
  Mat4x4f modelMatrix;
};

struct InstancedModelNode : public RenderNode
{
  InstancedModelNode()
    : RenderNode(RenderNodeType::instancedModel) {}

  RenderItemId mesh;
  RenderItemId material;
  std::vector<InstanceData> instances;
};

class InstancedPipeline
{
  public:
    InstancedPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass,
      VkDescriptorSetLayout uboDescriptorSetLayout,
      VkDescriptorSetLayout materialDescriptorSetLayout);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const MeshData& mesh,
      VkDescriptorSet uboDescriptorSet, VkDescriptorSet materialDescriptorSet, bool useMaterial);

    ~InstancedPipeline();

  private:
    VkDevice m_device;
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
};

using InstancedPipelinePtr = std::unique_ptr<InstancedPipeline>;
