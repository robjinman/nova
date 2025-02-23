#pragma once

#include "model.hpp"
#include <vulkan/vulkan.h>

struct MatricesUbo
{
  Mat4x4f viewMatrix;
  Mat4x4f projMatrix;
};

struct Light
{
  Vec3f worldPos;
  uint8_t _padding1[4];
  Vec3f colour;
  float_t ambient;
  //uint8_t _padding2[12];
};

struct LightingUbo
{
  Light lights[8];
  int32_t numLights;
};

struct MaterialUbo
{
  Vec3f colour;
  VkBool32 hasTexture;
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
    virtual RenderItemId addCubeMap(std::array<TexturePtr, 6> textures) = 0;
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

    // Matrices
    //
    virtual void updateMatricesUbo(const MatricesUbo& ubo, size_t currentFrame) = 0;
    virtual VkDescriptorSetLayout getMatricesDescriptorSetLayout() const = 0;
    virtual VkDescriptorSet getMatricesDescriptorSet(size_t currentFrame) const = 0;

    // Lighting
    //
    virtual void updateLightingUbo(const LightingUbo& ubo, size_t currentFrame) = 0;
    virtual VkDescriptorSetLayout getLightingDescriptorSetLayout() const = 0;
    virtual VkDescriptorSet getLightingDescriptorSet(size_t currentFrame) const = 0;

    virtual ~RenderResources() = default;
};

using RenderResourcesPtr = std::unique_ptr<RenderResources>;

class Logger;

RenderResourcesPtr createRenderResources(VkPhysicalDevice physicalDevice, VkDevice device,
  VkQueue graphicsQueue, VkCommandPool commandPool, Logger& logger);
