#include "render_system.hpp"
#include "renderer.hpp"
#include "spatial_system.hpp"
#include "logger.hpp"
#include "camera.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include <map>
#include <cassert>

namespace
{

class RenderSystemImpl : public RenderSystem
{
  public:
    RenderSystemImpl(const SpatialSystem&, Renderer& renderer, Logger& logger);

    void addComponent(ComponentPtr component) override;
    void removeComponent(EntityId entityId) override;
    bool hasComponent(EntityId entityId) const override;
    CRender& getComponent(EntityId entityId) override;
    const CRender& getComponent(EntityId entityId) const override;
    void update() override;

    RenderItemId addTexture(TexturePtr texture) override;
    RenderItemId addCubeMap(const std::array<TexturePtr, 6>& textures) override;
    RenderItemId addMaterial(MaterialPtr material) override;
    RenderItemId addMesh(MeshPtr mesh) override;

    void removeTexture(RenderItemId id) override;
    void removeCubeMap(RenderItemId id) override;
    void removeMaterial(RenderItemId id) override;
    void removeMesh(RenderItemId id) override;

    Camera& camera() override;
    const Camera& camera() const override;

  private:
    Logger& m_logger;
    Camera m_camera;
    const SpatialSystem& m_spatialSystem;
    Renderer& m_renderer;
    std::map<EntityId, CRenderPtr> m_components;
};

RenderSystemImpl::RenderSystemImpl(const SpatialSystem& spatialSystem, Renderer& renderer,
  Logger& logger)
  : m_logger(logger)
  , m_spatialSystem(spatialSystem)
  , m_renderer(renderer)
{
}

void RenderSystemImpl::addComponent(ComponentPtr component)
{
  auto renderComp = CRenderPtr(dynamic_cast<CRender*>(component.release()));
  m_components[renderComp->id()] = std::move(renderComp);
}

void RenderSystemImpl::removeComponent(EntityId entityId)
{
  m_components.erase(entityId);
}

bool RenderSystemImpl::hasComponent(EntityId entityId) const
{
  return m_components.count(entityId) > 0;
}

CRender& RenderSystemImpl::getComponent(EntityId entityId)
{
  return *m_components.at(entityId);
}

const CRender& RenderSystemImpl::getComponent(EntityId entityId) const
{
  return *m_components.at(entityId);
}

RenderItemId RenderSystemImpl::addTexture(TexturePtr texture)
{
  return m_renderer.addTexture(std::move(texture));
}

RenderItemId RenderSystemImpl::addMaterial(MaterialPtr material)
{
  return m_renderer.addMaterial(std::move(material));
}

RenderItemId RenderSystemImpl::addMesh(MeshPtr mesh)
{
  return m_renderer.addMesh(std::move(mesh));
}

void RenderSystemImpl::removeTexture(RenderItemId id)
{
  m_renderer.removeTexture(id);
}

void RenderSystemImpl::removeMaterial(RenderItemId id)
{
  m_renderer.removeMaterial(id);
}

void RenderSystemImpl::removeMesh(RenderItemId id)
{
  m_renderer.removeMesh(id);
}

Camera& RenderSystemImpl::camera()
{
  return m_camera;
}

const Camera& RenderSystemImpl::camera() const
{
  return m_camera;
}

void RenderSystemImpl::update()
{
  try {
    m_renderer.beginFrame(m_camera);

    for (auto& entry : m_components) {
      const auto& component = *entry.second;
      const auto& spatial = m_spatialSystem.getComponent(component.id());

      switch(component.type) {
        case CRenderType::instance:
          m_renderer.stageInstance(component.mesh, component.material, spatial.absTransform());
          break;
        case CRenderType::regular:
          m_renderer.stageModel(component.mesh, component.material, spatial.absTransform());
          break;
        case CRenderType::skybox:
          m_renderer.stageSkybox(component.mesh, component.material);
          break;
      }
    }

    m_renderer.endFrame();
  }
  catch(const std::exception& e) {
    EXCEPTION(STR("Error rendering scene; " << e.what()));
  }
}

} // namespace

RenderSystemPtr createRenderSystem(const SpatialSystem& spatialSystem, Renderer& renderer,
  Logger& logger)
{
  return std::make_unique<RenderSystemImpl>(spatialSystem, renderer, logger);
}
