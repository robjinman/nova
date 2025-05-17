#pragma once

#include "math.hpp"
#include "system.hpp"
#include "renderables.hpp"
#include <set>
#include <map>

struct Skin
{
  std::vector<size_t> joints; // Indices into skeleton joints
  std::vector<Mat4x4f> inverseBindMatrices;
};

using SkinPtr = std::unique_ptr<Skin>;

struct Transform
{
  std::optional<Vec4f> rotation;
  std::optional<Vec3f> translation;
  std::optional<Vec3f> scale;

  Mat4x4f toMatrix() const;
  void mix(const Transform& T);
};

struct AnimationChannel
{
  size_t jointIndex;  // Index into skin
  std::vector<float_t> timestamps;
  std::vector<Transform> transforms;
};

struct Animation
{
  std::string name;
  // TODO: Duration
  std::vector<AnimationChannel> channels;
};

using AnimationPtr = std::unique_ptr<Animation>;

struct Joint
{
  Mat4x4f transform;
  std::vector<size_t> children;
};

struct Skeleton
{
  size_t rootNodeIndex = 0;
  std::vector<Joint> joints;
};

using SkeletonPtr = std::unique_ptr<Skeleton>;

enum class CRenderType
{
  Model,
  Light,
  Skybox,
  ParticleEmitter
};

struct AnimationSet
{
  SkeletonPtr skeleton;
  std::map<std::string, AnimationPtr> animations;
};

using AnimationSetPtr = std::unique_ptr<AnimationSet>;

struct CRender : public Component
{
  CRender(EntityId entityId, CRenderType type)
    : Component(entityId)
    , type(type)
  {}

  CRenderType type;

  virtual ~CRender() = 0;
};

using CRenderPtr = std::unique_ptr<CRender>;

struct Submodel
{
  render::MeshHandle mesh;
  render::MaterialHandle material;
  SkinPtr skin;
};

struct CRenderModel : public CRender
{
  CRenderModel(EntityId entityId)
    : CRender(entityId, CRenderType::Model)
  {}

  CRenderModel(const CRenderModel& cpy, EntityId entityId)
    : CRender(entityId, CRenderType::Model)
  {
    isInstanced = cpy.isInstanced;
    animations = cpy.animations;
    for (auto& m : cpy.submodels) {
      submodels.push_back(Submodel{
        .mesh = m.mesh,
        .material = m.material,
        .skin = m.skin == nullptr ? nullptr : std::make_unique<Skin>(*m.skin)
      });
    }
  }

  bool isInstanced = false;
  RenderItemId animations = NULL_ID;
  std::vector<Submodel> submodels;
};

using CRenderModelPtr = std::unique_ptr<CRenderModel>;

struct CRenderSkybox : public CRender
{
  CRenderSkybox(EntityId entityId)
    : CRender(entityId, CRenderType::Skybox)
  {}

  Submodel model;
};

using CRenderSkyboxPtr = std::unique_ptr<CRenderSkybox>;

struct CRenderLight : public CRender
{
  CRenderLight(EntityId entityId)
    : CRender(entityId, CRenderType::Light)
  {}

  std::vector<Submodel> submodels;
  Vec3f colour;
  float_t ambient = 0.f;
  float_t specular = 0.f;
  float_t zFar = 1500.f; // TODO
};

using CRenderLightPtr = std::unique_ptr<CRenderLight>;

// TODO
struct CRenderParticleEmitter : public CRender
{
  CRenderParticleEmitter(EntityId entityId)
    : CRender(entityId, CRenderType::ParticleEmitter)
  {}
};

using CRenderParticleEmitterPtr = std::unique_ptr<CRenderParticleEmitter>;

class Camera;

class RenderSystem : public System
{
  public:
    virtual void start() = 0;
    virtual double frameRate() const = 0;

    virtual Camera& camera() = 0;
    virtual const Camera& camera() const = 0;

    CRender& getComponent(EntityId entityId) override = 0;
    const CRender& getComponent(EntityId entityId) const override = 0;

    // Animations
    //
    virtual RenderItemId addAnimations(AnimationSetPtr animations) = 0;
    virtual void removeAnimations(RenderItemId id) = 0;
    virtual void playAnimation(EntityId entityId, const std::string& name) = 0;

    virtual ~RenderSystem() {}

    //
    // Pass through to Renderer
    // ------------------------

    // Initialisation
    //
    virtual void compileShader(const render::MeshFeatureSet& meshFeatures,
      const render::MaterialFeatureSet& materialFeatures) = 0;

    // Resources
    //
    virtual RenderItemId addTexture(render::TexturePtr texture) = 0;
    virtual RenderItemId addNormalMap(render::TexturePtr texture) = 0;
    virtual RenderItemId addCubeMap(std::array<render::TexturePtr, 6>&& textures) = 0;

    virtual void removeTexture(RenderItemId id) = 0;
    virtual void removeCubeMap(RenderItemId id) = 0;

    // Meshes
    //
    virtual render::MeshHandle addMesh(render::MeshPtr mesh) = 0;
    virtual void removeMesh(RenderItemId id) = 0;

    // Materials
    //
    virtual render::MaterialHandle addMaterial(render::MaterialPtr material) = 0;
    virtual void removeMaterial(RenderItemId id) = 0;
};

using RenderSystemPtr = std::unique_ptr<RenderSystem>;

class SpatialSystem;
namespace render { class Renderer; }
class Logger;

RenderSystemPtr createRenderSystem(const SpatialSystem& spatialSystem, render::Renderer& renderer,
  Logger& logger);
