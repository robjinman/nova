#pragma once

#include "math.hpp"
#include "system.hpp"
#include "model.hpp"
#include <set>
#include <memory>

struct CRender : public Component
{
  CRender(EntityId entityId)
    : Component(entityId) {}

  MeshId mesh = NULL_ID;
  MaterialId material = NULL_ID;
  bool isInstance = false;
};

using CRenderPtr = std::unique_ptr<CRender>;

class Camera;

class RenderSystem : public System
{
  public:
    CRender& getComponent(EntityId entityId) override = 0;
    const CRender& getComponent(EntityId entityId) const override = 0;

    virtual TextureId addTexture(TexturePtr texture) = 0;
    virtual MaterialId addMaterial(MaterialPtr material) = 0;
    virtual MeshId addMesh(MeshPtr mesh) = 0;

    virtual void removeTexture(TextureId id) = 0;
    virtual void removeMaterial(MaterialId id) = 0;
    virtual void removeMesh(MeshId id) = 0;

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
