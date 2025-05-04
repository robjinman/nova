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

    void start() override;
    double frameRate() const override;

    void addComponent(ComponentPtr component) override;
    void removeComponent(EntityId entityId) override;
    bool hasComponent(EntityId entityId) const override;
    CRender& getComponent(EntityId entityId) override;
    const CRender& getComponent(EntityId entityId) const override;
    void update() override;

    // Initialisation
    //

    void compileShader(const MeshFeatureSet& meshFeatures,
      const MaterialFeatureSet& materialFeatures) override;

    // Resources
    //
    RenderItemId addTexture(TexturePtr texture) override;
    RenderItemId addNormalMap(TexturePtr texture) override;
    RenderItemId addCubeMap(std::array<TexturePtr, 6>&& textures) override;

    void removeTexture(RenderItemId id) override;
    void removeCubeMap(RenderItemId id) override;

    // Meshes
    //
    MeshHandle addMesh(MeshPtr mesh) override;
    void removeMesh(RenderItemId id) override;

    // Materials
    //
    MaterialHandle addMaterial(MaterialPtr material) override;
    void removeMaterial(RenderItemId id) override;

    Camera& camera() override;
    const Camera& camera() const override;

  private:
    Logger& m_logger;
    Camera m_camera;
    const SpatialSystem& m_spatialSystem;
    Renderer& m_renderer;
    std::map<EntityId, CRenderPtr> m_components;
    std::set<EntityId> m_lights;

    std::vector<Vec2f> computeFrustumPerimeter(const Vec3f& viewPos, const Vec3f& viewDir,
      float_t hFov) const;
    void drawEntities(const std::unordered_set<EntityId>& entities);
    void doShadowPass();
    void doMainPass();
};

RenderSystemImpl::RenderSystemImpl(const SpatialSystem& spatialSystem, Renderer& renderer,
  Logger& logger)
  : m_logger(logger)
  , m_spatialSystem(spatialSystem)
  , m_renderer(renderer)
{
}

void RenderSystemImpl::compileShader(const MeshFeatureSet& meshFeatures,
  const MaterialFeatureSet& materialFeatures)
{
  m_renderer.compileShader(meshFeatures, materialFeatures);
}

std::vector<Vec2f> RenderSystemImpl::computeFrustumPerimeter(const Vec3f& viewPos,
  const Vec3f& viewDir, float_t hFov) const
{
  auto params = m_renderer.getViewParams();
  Vec3f A{ params.nearPlane * static_cast<float_t>(tan(0.5f * hFov)), params.nearPlane, 1 };
  Vec3f B{ params.farPlane * static_cast<float_t>(tan(0.5f * hFov)), params.farPlane, 1 };
  Vec3f C{ -B[0], B[1], 1 };
  Vec3f D{ -A[0], A[1], 1 };

  float_t a = atan2(viewDir[2], viewDir[0]) - 0.5f * PIf;

  Mat3x3f m{
    cosine(a), -sine(a), viewPos[0],
    sine(a), cosine(a), viewPos[2],
    0, 0, 1
  };

  return std::vector<Vec2f>{(m * A).sub<2>(), (m * B).sub<2>(), (m * C).sub<2>(), (m * D).sub<2>()};
}

void RenderSystemImpl::start()
{
  m_renderer.start();
}

double RenderSystemImpl::frameRate() const
{
  return m_renderer.frameRate();
}

void RenderSystemImpl::addComponent(ComponentPtr component)
{
  auto renderComp = CRenderPtr(dynamic_cast<CRender*>(component.release()));
  if (renderComp->type == CRenderType::Light) {
    m_lights.insert(renderComp->id());
  }
  m_components[renderComp->id()] = std::move(renderComp);
}

void RenderSystemImpl::removeComponent(EntityId entityId)
{
  auto i = m_components.find(entityId);
  if (i != m_components.end()) {
    if (i->second->type == CRenderType::Light) {
      m_lights.erase(entityId);
    }
    m_components.erase(i);
  }
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

RenderItemId RenderSystemImpl::addNormalMap(TexturePtr texture)
{
  return m_renderer.addNormalMap(std::move(texture));
}

RenderItemId RenderSystemImpl::addCubeMap(std::array<TexturePtr, 6>&& textures)
{
  return m_renderer.addCubeMap(std::move(textures));
}

MaterialHandle RenderSystemImpl::addMaterial(MaterialPtr material)
{
  return m_renderer.addMaterial(std::move(material));
}

MeshHandle RenderSystemImpl::addMesh(MeshPtr mesh)
{
  return m_renderer.addMesh(std::move(mesh));
}

void RenderSystemImpl::removeTexture(RenderItemId id)
{
  m_renderer.removeTexture(id);
}

void RenderSystemImpl::removeCubeMap(RenderItemId id)
{
  m_renderer.removeCubeMap(id);
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

void RenderSystemImpl::drawEntities(const std::unordered_set<EntityId>& entities)
{
  for (EntityId id : entities) {
    auto entry = m_components.find(id);
    if (entry == m_components.end()) {
      continue;
    }

    const auto& component = *entry->second;
    const auto& spatial = m_spatialSystem.getComponent(id);

    switch(component.type) {
      case CRenderType::Instance: {
        for (auto& mesh : component.meshes) {
          m_renderer.drawInstance(mesh.mesh, mesh.material, spatial.absTransform());
        }
        break;
      }
      case CRenderType::Regular: {
        for (auto& mesh : component.meshes) {
          m_renderer.drawModel(mesh.mesh, mesh.material,
            spatial.absTransform() * mesh.mesh.transform);
        }
        break;
      }
      case CRenderType::Skybox: {
        ASSERT(component.meshes.size() == 1, "Expected skybox to have exactly 1 mesh");
        m_renderer.drawSkybox(component.meshes[0].mesh, component.meshes[0].material);
        break;
      }
      case CRenderType::Light: break;
    }
  }
}

void RenderSystemImpl::doShadowPass()
{
  m_renderer.beginPass(RenderPass::Shadow, m_camera.getMatrix());

  // TODO: Separate pass for every shadow casting light
  const CRenderLight& firstLight =
    dynamic_cast<const CRenderLight&>(*m_components.at(*m_lights.begin()));
  const CSpatial& firstLightSpatial = m_spatialSystem.getComponent(firstLight.id());
  auto firstLightPos = getTranslation(firstLightSpatial.absTransform());

  // TODO
  auto frustum = computeFrustumPerimeter(firstLightPos, Vec3f{ 1.f, -0.2f, 1.f }.normalise(),
    degreesToRadians(90.f));
  auto visible = m_spatialSystem.getIntersecting(frustum);

  drawEntities(visible);

  m_renderer.endPass();
}

void RenderSystemImpl::doMainPass()
{
  m_renderer.beginPass(RenderPass::Main, m_camera.getMatrix());

  auto frustum = computeFrustumPerimeter(m_camera.getPosition(), m_camera.getDirection(),
    m_renderer.getViewParams().hFov);
  auto visible = m_spatialSystem.getIntersecting(frustum);

  drawEntities(visible);

  for (EntityId id : m_lights) {
    const CRenderLight& light = dynamic_cast<const CRenderLight&>(*m_components.at(id));
    const auto& spatial = m_spatialSystem.getComponent(id);
    const auto& transform = spatial.absTransform();

    m_renderer.drawLight(light.colour, light.ambient, light.specular, getTranslation(transform));

    if (light.meshes.size() > 0) {
      for (auto& mesh : light.meshes) {
        m_renderer.drawModel(mesh.mesh, mesh.material, spatial.absTransform());
      }
    }
  }

  m_renderer.endPass();
}

void RenderSystemImpl::update()
{
  try {
    m_renderer.beginFrame();

    doShadowPass();
    doMainPass();

    m_renderer.endFrame();
    m_renderer.checkError();
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
