#pragma once

#include "vulkan/pipeline.hpp"
#include "vulkan/render_resources.hpp"

struct SkyboxNode : public RenderNode
{
  SkyboxNode()
    : RenderNode(RenderNodeType::Skybox) {}

  RenderItemId mesh;
  RenderItemId material;
};

class FileSystem;

class SkyboxPipeline : public Pipeline
{
  public:
    SkyboxPipeline(const FileSystem& fileSystem, VkDevice device, VkExtent2D swapchainExtent,
      VkRenderPass renderPass, const RenderResources& renderResources);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
      size_t currentFrame) override;

    ~SkyboxPipeline();

  private:
    VkDevice m_device;
    const RenderResources& m_renderResources;
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
};

using SkyboxPipelinePtr = std::unique_ptr<SkyboxPipeline>;
