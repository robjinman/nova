#pragma once

#include "tree_set.hpp"
#include "model.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

enum class RenderNodeType
{
  defaultModel,
  instancedModel
};

struct RenderNode
{
  RenderNode(RenderNodeType type)
    : type(type) {}

  RenderNodeType type;

  virtual ~RenderNode() {}
};

using RenderNodePtr = std::unique_ptr<RenderNode>;
using RenderGraph = TreeSet<long, RenderNodePtr>;

// TODO: Support different types of models
struct ModelData
{
  ModelPtr model;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  VkBuffer instanceBuffer;
  VkDeviceMemory instanceBufferMemory;
  size_t numInstances;
};

using ModelDataPtr = std::unique_ptr<ModelData>;

class Pipeline
{
  public:
    virtual ~Pipeline() {}
};

using PipelinePtr = std::unique_ptr<Pipeline>;

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

PipelinePtr createInstancedPipeline(VkDevice device, VkExtent2D swapchainExtent,
  VkRenderPass renderPass);
