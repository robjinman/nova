#pragma once

#include "model.hpp"
#include <vulkan/vulkan.h>

struct UniformBufferObject
{
  Mat4x4f viewMatrix;
  Mat4x4f projMatrix;
};

struct MeshInstance
{
  Mat4x4f modelMatrix;
};

struct MeshBuffers
{
  VkBuffer vertexBuffer;
  VkBuffer indexBuffer;
  VkBuffer instanceBuffer;
  size_t numIndices;
  size_t numInstances;
};

class RenderResources
{
  public:
    // Resources
    //
    virtual RenderItemId addTexture(TexturePtr texture) = 0;
    virtual RenderItemId addCubeMap(const std::array<TexturePtr, 6>& textures) = 0;
    virtual void removeTexture(RenderItemId id) = 0;
    virtual void removeCubeMap(RenderItemId id) = 0;

    // Meshes
    //
    virtual RenderItemId addMesh(MeshPtr mesh) = 0;
    virtual void removeMesh(RenderItemId id) = 0;
    virtual MeshBuffers getMeshBuffers(RenderItemId id) const = 0;
    virtual void updateMeshInstances(RenderItemId id,
      const std::vector<MeshInstance>& instances) = 0;

    // Materials
    //
    virtual RenderItemId addMaterial(MaterialPtr material) = 0;
    virtual void removeMaterial(RenderItemId id) = 0;
    virtual VkDescriptorSetLayout getMaterialDescriptorSetLayout() const = 0;
    virtual VkDescriptorSet getMaterialDescriptorSet(RenderItemId id) const = 0;

    // UBO
    //
    virtual void updateUniformBuffer(const UniformBufferObject& ubo, size_t currentFrame) = 0;
    virtual VkDescriptorSetLayout getUboDescriptorSetLayout() const = 0;
    virtual VkDescriptorSet getUboDescriptorSet(size_t currentFrame) const = 0;

    virtual ~RenderResources() = default;
};

using RenderResourcesPtr = std::unique_ptr<RenderResources>;

RenderResourcesPtr createRenderResources(VkPhysicalDevice physicalDevice, VkDevice device,
  VkQueue graphicsQueue, VkCommandPool commandPool);
