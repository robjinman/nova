#pragma once

#include "model.hpp"
#include <vulkan/vulkan.h>

const uint32_t MAX_LIGHTS = 4;
const uint32_t SHADOW_MAP_W = 2048;
const uint32_t SHADOW_MAP_H = 2048;

#pragma pack(push, 4)
struct CameraTransformsUbo
{
  Mat4x4f viewMatrix;
  Mat4x4f projMatrix;
};

struct LightTransformsUbo
{
  Mat4x4f viewMatrix;
  Mat4x4f projMatrix;
};

struct Light
{
  Vec3f worldPos;
  uint8_t _pad0[4];
  Vec3f colour;
  float_t ambient;
  float_t specular;
  uint8_t _pad2[12];
};

struct LightingUbo
{
  Vec3f cameraPos;
  int32_t numLights;
  Light lights[MAX_LIGHTS];
};

struct MaterialUbo
{
  Vec4f colour;
  // TODO: PBR properties
};

struct MeshInstance
{
  Mat4x4f modelMatrix;
};
#pragma pack(pop)

struct MeshBuffers
{
  VkBuffer vertexBuffer;
  VkBuffer indexBuffer;
  VkBuffer instanceBuffer;
  uint32_t numIndices;
  uint32_t numInstances;
};

class RenderResources
{
  public:
    // Resources
    //
    virtual RenderItemId addTexture(TexturePtr texture) = 0;
    virtual RenderItemId addNormalMap(TexturePtr texture) = 0;
    virtual RenderItemId addCubeMap(std::array<TexturePtr, 6> textures) = 0;
    virtual void removeTexture(RenderItemId id) = 0;
    virtual void removeCubeMap(RenderItemId id) = 0;

    // Meshes
    //
    virtual MeshHandle addMesh(MeshPtr mesh) = 0;
    virtual void removeMesh(RenderItemId id) = 0;
    virtual MeshBuffers getMeshBuffers(RenderItemId id) const = 0;
    virtual void updateMeshInstances(RenderItemId id,
      const std::vector<MeshInstance>& instances) = 0;
    virtual const MeshFeatureSet& getMeshFeatures(RenderItemId id) const = 0;

    // Materials
    //
    virtual MaterialHandle addMaterial(MaterialPtr material) = 0;
    virtual void removeMaterial(RenderItemId id) = 0;
    virtual VkDescriptorSetLayout getMaterialDescriptorSetLayout() const = 0;
    virtual VkDescriptorSet getMaterialDescriptorSet(RenderItemId id) const = 0;
    virtual const MaterialFeatureSet& getMaterialFeatures(RenderItemId id) const = 0;

    // Transforms
    //
    virtual VkDescriptorSetLayout getTransformsDescriptorSetLayout() const = 0;
    virtual VkDescriptorSet getTransformsDescriptorSet(size_t currentFrame) const = 0;
    // > Camera
    virtual void updateCameraTransformsUbo(const CameraTransformsUbo& ubo, size_t currentFrame) = 0;
    // > Light
    virtual void updateLightTransformsUbo(const LightTransformsUbo& ubo, size_t currentFrame) = 0;

    // Lighting
    //
    virtual void updateLightingUbo(const LightingUbo& ubo, size_t currentFrame) = 0;
    virtual VkDescriptorSetLayout getLightingDescriptorSetLayout() const = 0;
    virtual VkDescriptorSet getLightingDescriptorSet(size_t currentFrame) const = 0;

    // Shadow pass
    //
    virtual VkDescriptorSetLayout getShadowPassDescriptorSetLayout() const = 0;
    virtual VkDescriptorSet getShadowPassDescriptorSet() const = 0;
    virtual VkImage getShadowMapImage() const = 0;
    virtual VkImageView getShadowMapImageView() const = 0;

    virtual ~RenderResources() = default;
};

using RenderResourcesPtr = std::unique_ptr<RenderResources>;

class Logger;

RenderResourcesPtr createRenderResources(VkPhysicalDevice physicalDevice, VkDevice device,
  VkQueue graphicsQueue, VkCommandPool commandPool, Logger& logger);
