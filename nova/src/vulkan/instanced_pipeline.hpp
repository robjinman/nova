#pragma once

#include "vulkan/pipeline.hpp"
#include "vulkan/render_resources.hpp"

struct InstancedModelNode : public RenderNode
{
  InstancedModelNode()
    : RenderNode(RenderNodeType::instancedModel) {}

  RenderItemId mesh;
  RenderItemId material;
  std::vector<MeshInstance> instances;
};

class FileSystem;

class InstancedPipeline : public Pipeline
{
  public:
    InstancedPipeline(const FileSystem& fileSystem, VkDevice device, VkExtent2D swapchainExtent,
      VkRenderPass renderPass, const RenderResources& renderResources);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
      size_t currentFrame) override;

    ~InstancedPipeline();

  private:
    VkDevice m_device;
    const RenderResources& m_renderResources;
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
};

using InstancedPipelinePtr = std::unique_ptr<InstancedPipeline>;
