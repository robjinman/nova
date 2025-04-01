#pragma once

#include "math.hpp"
#include "system.hpp"
#include "model.hpp"
#include <set>
#include <memory>

enum class CRenderType
{
  Regular,
  Instance,
  Light,
  Skybox
};

struct MeshMaterialPair
{
  RenderItemId mesh = NULL_ID;
  RenderItemId material = NULL_ID;
};

struct CRender : public Component
{
  CRender(EntityId entityId, CRenderType type)
    : Component(entityId)
    , type(type)
  {}

  CRenderType type;
  std::vector<MeshMaterialPair> meshes;
};

struct CRenderLight : public CRender
{
  CRenderLight(EntityId entityId)
    : CRender(entityId, CRenderType::Light)
  {}

  Vec3f colour;
  float_t ambient = 0.f;
};

using CRenderPtr = std::unique_ptr<CRender>;

class Camera;

class RenderSystem : public System
{
  public:
    virtual void start() = 0;
    virtual double frameRate() const = 0;

    CRender& getComponent(EntityId entityId) override = 0;
    const CRender& getComponent(EntityId entityId) const override = 0;

    // Resources
    //
    virtual RenderItemId addTexture(TexturePtr texture) = 0;
    virtual RenderItemId addCubeMap(std::array<TexturePtr, 6>&& textures) = 0;

    virtual void removeTexture(RenderItemId id) = 0;
    virtual void removeCubeMap(RenderItemId id) = 0;

    // Meshes
    //
    virtual RenderItemId addMesh(MeshPtr mesh) = 0;
    virtual void removeMesh(RenderItemId id) = 0;

    // Materials
    //
    virtual RenderItemId addMaterial(MaterialPtr material) = 0;
    virtual void removeMaterial(RenderItemId id) = 0;

    virtual Camera& camera() = 0;
    virtual const Camera& camera() const = 0;

    virtual ~RenderSystem() {}
};

using RenderSystemPtr = std::unique_ptr<RenderSystem>;

class SpatialSystem;
class Renderer;
class Logger;

RenderSystemPtr createRenderSystem(const SpatialSystem& spatialSystem, Renderer& renderer,
  Logger& logger);
