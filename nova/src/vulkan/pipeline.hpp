#pragma once

#include "tree_set.hpp"
#include "model.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

enum class RenderNodeType
{
  DefaultModel,
  InstancedModel,
  Skybox
};

struct RenderNode
{
  RenderNode(RenderNodeType type)
    : type(type) {}

  RenderNodeType type;
  RenderItemId mesh;
  RenderItemId material;

  virtual ~RenderNode() = default;
};

using RenderGraphKey = long;
using RenderNodePtr = std::unique_ptr<RenderNode>;
using RenderGraph = TreeSet<RenderGraphKey, RenderNodePtr>;

struct PipelineKey
{
  MeshFeatureSet meshFeatures;
  MaterialFeatureSet materialFeatures;

  bool operator==(const PipelineKey& rhs) const = default;
};

template<>
struct std::hash<PipelineKey>
{
  std::size_t operator()(const PipelineKey& key) const noexcept
  {
    std::string_view stringView{reinterpret_cast<const char*>(&key), sizeof(PipelineKey)};
    return std::hash<std::string_view>{}(stringView);
  }
};

// TODO: Move bindstate into Pipeline class when we only have a single Pipeline class
struct BindState
{
  VkPipeline pipeline;
};

class Pipeline
{
  public:
    virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
      BindState& bindState, size_t currentFrame) = 0;

    virtual ~Pipeline() = default;
};

using PipelinePtr = std::unique_ptr<Pipeline>;

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

VkVertexInputBindingDescription defaultVertexBindingDescription();

std::vector<VkVertexInputAttributeDescription> defaultAttributeDescriptions();

VkPipelineInputAssemblyStateCreateInfo defaultInputAssemblyState();

VkPipelineViewportStateCreateInfo defaultViewportState(VkViewport& viewport, VkRect2D& scissor,
  VkExtent2D swapchainExtent);

VkPipelineRasterizationStateCreateInfo defaultRasterizationState();

VkPipelineMultisampleStateCreateInfo defaultMultisamplingState();

VkPipelineColorBlendStateCreateInfo
defaultColourBlendState(VkPipelineColorBlendAttachmentState& colourBlendAttachment);

VkPipelineDepthStencilStateCreateInfo defaultDepthStencilState();
