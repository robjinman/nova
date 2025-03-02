#pragma once

#include "vulkan/pipeline.hpp"
#include "vulkan/render_resources.hpp"

struct DefaultModelNode : public RenderNode
{
  DefaultModelNode()
    : RenderNode(RenderNodeType::defaultModel) {}

  RenderItemId mesh;
  RenderItemId material;
  Mat4x4f modelMatrix;
};

class PlatformPaths;

class DefaultPipeline : public Pipeline
{
  public:
    DefaultPipeline(const PlatformPaths& platformPaths, VkDevice device, VkExtent2D swapchainExtent,
      VkRenderPass renderPass, const RenderResources& renderResources);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
      size_t currentFrame) override;

    ~DefaultPipeline();

  private:
    VkDevice m_device;
    const RenderResources& m_renderResources;
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
};

using DefaultPipelinePtr = std::unique_ptr<DefaultPipeline>;
