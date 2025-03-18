#pragma once

#include "vulkan/pipeline.hpp"
#include "vulkan/render_resources.hpp"

struct DefaultModelNode : public RenderNode
{
  DefaultModelNode()
    : RenderNode(RenderNodeType::DefaultModel)
  {}

  RenderItemId mesh;
  RenderItemId material;
  Mat4x4f modelMatrix;
};

class FileSystem;

class DefaultPipeline : public Pipeline
{
  public:
    DefaultPipeline(const FileSystem& fileSystem, VkDevice device, VkExtent2D swapchainExtent,
      VkRenderPass renderPass, const RenderResources& renderResources);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
      BindState& bindState, size_t currentFrame) override;

    ~DefaultPipeline();

  private:
    VkDevice m_device;
    const RenderResources& m_renderResources;
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
};

using DefaultPipelinePtr = std::unique_ptr<DefaultPipeline>;
