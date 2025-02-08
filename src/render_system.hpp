#pragma once

#include "math.hpp"
#include "system.hpp"
#include "model.hpp"
#include <set>
#include <memory>

enum class CRenderType
{
  regular,
  instance,
  skybox
};

struct CRender : public Component
{
  CRender(EntityId entityId, CRenderType type)
    : Component(entityId)
    , type(type) {}

  // TODO: Consider subclassing this
  RenderItemId mesh = NULL_ID;
  RenderItemId material = NULL_ID;
  RenderItemId cubeMap = NULL_ID;
  CRenderType type;
};

using CRenderPtr = std::unique_ptr<CRender>;

class Camera;

class RenderSystem : public System
{
  public:
    CRender& getComponent(EntityId entityId) override = 0;
    const CRender& getComponent(EntityId entityId) const override = 0;

    virtual RenderItemId addTexture(TexturePtr texture) = 0;
    virtual RenderItemId addCubeMap(const std::array<TexturePtr, 6>& textures) = 0;
    virtual RenderItemId addMaterial(MaterialPtr material) = 0;
    virtual RenderItemId addMesh(MeshPtr mesh) = 0;

    virtual void removeTexture(RenderItemId id) = 0;
    virtual void removeCubeMap(RenderItemId id) = 0;
    virtual void removeMaterial(RenderItemId id) = 0;
    virtual void removeMesh(RenderItemId id) = 0;

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
