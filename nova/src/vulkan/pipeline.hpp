#pragma once

#include "vulkan/pipeline.hpp"
#include "vulkan/render_resources.hpp"
#include "tree_set.hpp"

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

struct DefaultModelNode : public RenderNode
{
  DefaultModelNode()
    : RenderNode(RenderNodeType::DefaultModel)
  {}

  Mat4x4f modelMatrix;
};

struct InstancedModelNode : public RenderNode
{
  InstancedModelNode()
    : RenderNode(RenderNodeType::InstancedModel)
  {}

  std::vector<MeshInstance> instances;
};

struct SkyboxNode : public RenderNode
{
  SkyboxNode()
    : RenderNode(RenderNodeType::Skybox)
  {}
};

struct BindState
{
  VkPipeline pipeline;
};

class FileSystem;

class Pipeline
{
  public:
    Pipeline(const MeshFeatureSet& meshFeatures, const MaterialFeatureSet& materialFeatures,
      const FileSystem& fileSystem, VkDevice device, VkExtent2D swapchainExtent,
      VkRenderPass renderPass, const RenderResources& renderResources);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, const RenderNode& node,
      BindState& bindState, size_t currentFrame);

    ~Pipeline();

  private:
    VkDevice m_device;
    const RenderResources& m_renderResources;
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
};

using PipelinePtr = std::unique_ptr<Pipeline>;
