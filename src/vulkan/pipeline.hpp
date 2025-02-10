#pragma once

#include "tree_set.hpp"
#include "model.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

enum class RenderNodeType
{
  defaultModel,
  instancedModel,
  skybox
};

struct RenderNode
{
  RenderNode(RenderNodeType type)
    : type(type) {}

  RenderNodeType type;

  virtual ~RenderNode() = default;
};

using RenderGraphKey = long;
using RenderNodePtr = std::unique_ptr<RenderNode>;
using RenderGraph = TreeSet<RenderGraphKey, RenderNodePtr>;

class Pipeline
{
  public:
    virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
      size_t currentFrame) = 0;

    virtual ~Pipeline() = default;
};

using PipelinePtr = std::unique_ptr<Pipeline>;

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
VkVertexInputBindingDescription getDefaultVertexBindingDescription();
std::vector<VkVertexInputAttributeDescription> getDefaultAttributeDescriptions();
