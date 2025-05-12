#pragma once

#include "renderer.hpp"
#include <vulkan/vulkan.h>

class Logger;

namespace render
{

const uint32_t MAX_LIGHTS = 4;
const uint32_t SHADOW_MAP_W = 4096;
const uint32_t SHADOW_MAP_H = 4096;
const uint32_t MAX_JOINTS = 128;

// TODO: Hide these inside cpp file?
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
  Vec3f viewPos;
  uint32_t numLights;
  Light lights[MAX_LIGHTS];
};

struct MaterialUbo
{
  Vec4f colour;
  // TODO: PBR properties
};

struct JointTransformsUbo
{
  Mat4x4f transforms[MAX_JOINTS];
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

enum class DescriptorSetNumber : uint32_t
{
  Global = 0,
  RenderPass = 1,
  Material = 2,
  Object = 3
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

    // Descriptor sets
    //
    virtual VkDescriptorSetLayout getDescriptorSetLayout(DescriptorSetNumber number) const = 0;
    virtual VkDescriptorSet getGlobalDescriptorSet(size_t currentFrame) const = 0;
    virtual VkDescriptorSet getRenderPassDescriptorSet(RenderPass renderpass,
      size_t currentFrame) const = 0;
    virtual VkDescriptorSet getMaterialDescriptorSet(RenderItemId id) const = 0;
    virtual VkDescriptorSet getObjectDescriptorSet(RenderItemId id, size_t currentFrame) const = 0;

    // Meshes
    //
    virtual MeshHandle addMesh(MeshPtr mesh) = 0;
    virtual void removeMesh(RenderItemId id) = 0;
    virtual void updateJointTransforms(RenderItemId meshId, const std::vector<Mat4x4f>& joints,
      size_t currentFrame) = 0;
    virtual MeshBuffers getMeshBuffers(RenderItemId id) const = 0;
    virtual void updateMeshInstances(RenderItemId id,
      const std::vector<MeshInstance>& instances) = 0;
    virtual const MeshFeatureSet& getMeshFeatures(RenderItemId id) const = 0;

    // Materials
    //
    virtual MaterialHandle addMaterial(MaterialPtr material) = 0;
    virtual void removeMaterial(RenderItemId id) = 0;
    virtual const MaterialFeatureSet& getMaterialFeatures(RenderItemId id) const = 0;

    // Transforms
    //
    // > Camera
    virtual void updateCameraTransformsUbo(const CameraTransformsUbo& ubo, size_t currentFrame) = 0;
    // > Light
    virtual void updateLightTransformsUbo(const LightTransformsUbo& ubo, size_t currentFrame) = 0;

    // Lighting
    //
    virtual void updateLightingUbo(const LightingUbo& ubo, size_t currentFrame) = 0;

    // Shadow pass
    //
    virtual VkImage getShadowMapImage() const = 0;
    virtual VkImageView getShadowMapImageView() const = 0;

    virtual ~RenderResources() = default;
};

using RenderResourcesPtr = std::unique_ptr<RenderResources>;

RenderResourcesPtr createRenderResources(VkPhysicalDevice physicalDevice, VkDevice device,
  VkQueue graphicsQueue, VkCommandPool commandPool, Logger& logger);

} // namespace render
