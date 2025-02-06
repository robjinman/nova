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

struct MeshData
{
  MeshPtr mesh;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  VkBuffer instanceBuffer;
  VkDeviceMemory instanceBufferMemory;
  size_t numInstances;
};

using MeshDataPtr = std::unique_ptr<MeshData>;

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
VkVertexInputBindingDescription getDefaultVertexBindingDescription();
std::vector<VkVertexInputAttributeDescription> getDefaultAttributeDescriptions();
