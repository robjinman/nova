#include "render_system.hpp"
#include "renderer.hpp"
#include "spatial_system.hpp"
#include "logger.hpp"
#include "camera.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include "time.hpp"
#include <map>
#include <cassert>
#include <functional>
#include <iostream> // TODO

using render::Renderer;
using render::MeshPtr;
using render::MaterialPtr;
using render::MeshFeatureSet;
using render::MaterialFeatureSet;
using render::MeshHandle;
namespace MeshFeatures = render::MeshFeatures;
using render::MaterialHandle;
using render::TexturePtr;
using render::RenderPass;

CRender::~CRender() {}

namespace
{

struct AnimationChannelState
{
  bool stopped = false;
  size_t frame;
};

struct AnimationState
{
  RenderItemId animationSet = NULL_ID;
  std::string animationName;
  Timer timer;
  std::vector<AnimationChannelState> channels;
};

class RenderSystemImpl : public RenderSystem
{
  public:
    RenderSystemImpl(const SpatialSystem&, Renderer& renderer, Logger& logger);

    void start() override;
    double frameRate() const override;

    Camera& camera() override;
    const Camera& camera() const override;

    void addComponent(ComponentPtr component) override;
    void removeComponent(EntityId entityId) override;
    bool hasComponent(EntityId entityId) const override;
    CRender& getComponent(EntityId entityId) override;
    const CRender& getComponent(EntityId entityId) const override;
    void update() override;

    // Animations
    //
    RenderItemId addAnimations(AnimationSetPtr animations) override;
    void removeAnimations(RenderItemId id) override;
    void playAnimation(EntityId entityId, const std::string& name) override;

    //
    // Pass through to Renderer
    // ------------------------

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

  private:
    Logger& m_logger;
    Camera m_camera;
    const SpatialSystem& m_spatialSystem;
    Renderer& m_renderer;
    std::map<EntityId, CRenderPtr> m_components;
    std::set<EntityId> m_lights;
    std::map<RenderItemId, AnimationSetPtr> m_animationSets;
    std::map<EntityId, AnimationState> m_animationStates;

    using DrawFilter = std::function<bool(const Submodel&)>;

    std::vector<Vec2f> computePerspectiveFrustumPerimeter(const Vec3f& viewPos,
      const Vec3f& viewDir, float_t hFov) const;
    std::vector<Vec2f> computeOrthographicFrustumPerimeter(const Vec3f& viewPos,
      const Vec3f& viewDir, float_t hFov, float_t zFar) const;
    void drawEntities(const std::unordered_set<EntityId>& entities,
      const DrawFilter& filter = [](const Submodel&) { return true; });
    void doShadowPass();
    void doMainPass();
    void updateAnimations();
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

std::vector<Vec2f> RenderSystemImpl::computePerspectiveFrustumPerimeter(const Vec3f& viewPos,
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

std::vector<Vec2f> RenderSystemImpl::computeOrthographicFrustumPerimeter(const Vec3f& viewPos,
  const Vec3f& viewDir, float_t hFov, float_t zFar) const
{
  float_t w = zFar * tan(0.5f * hFov);
  Vec3f A{ w, 0.f, 1 };
  Vec3f B{ w, zFar, 1 };
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

RenderItemId RenderSystemImpl::addAnimations(AnimationSetPtr animations)
{
  static RenderItemId nextId = 0;

  auto id = nextId++;
  m_animationSets[id] = std::move(animations);

  return id;
}

void RenderSystemImpl::removeAnimations(RenderItemId id)
{
  // TODO
}

void RenderSystemImpl::playAnimation(EntityId entityId, const std::string& name)
{
  auto& component = *m_components.at(entityId);
  DBG_ASSERT(component.type == CRenderType::Model, "Can only play animation on models");
  auto& model = dynamic_cast<const CRenderModel&>(component);

  m_animationStates[entityId] = AnimationState{
    .animationSet = model.animations,
    .animationName = name,
    .timer{},
    .channels{}
  };
}

Camera& RenderSystemImpl::camera()
{
  return m_camera;
}

const Camera& RenderSystemImpl::camera() const
{
  return m_camera;
}

void RenderSystemImpl::drawEntities(const std::unordered_set<EntityId>& entities,
  const std::function<bool(const Submodel&)>& filter)
{
  for (EntityId id : entities) {
    auto entry = m_components.find(id);
    if (entry == m_components.end()) {
      continue;
    }

    const auto& component = *entry->second;
    const auto& spatial = m_spatialSystem.getComponent(id);

    switch(component.type) {
      case CRenderType::Model: {
        auto& model = dynamic_cast<const CRenderModel&>(component);
        for (auto& submodel : model.submodels) {
          if (filter(submodel)) {
            if (model.isInstanced) {
              m_renderer.drawInstance(submodel.mesh, submodel.material, spatial.absTransform());
            }
            else {
              m_renderer.drawModel(submodel.mesh, submodel.material,
                spatial.absTransform() * submodel.mesh.transform);
            }
          }
        }
        break;
      }
      case CRenderType::Skybox: {
        auto& skybox = dynamic_cast<const CRenderSkybox&>(component);
        if (filter(skybox.model)) {
          m_renderer.drawSkybox(skybox.model.mesh, skybox.model.material);
        }
        break;
      }
      case CRenderType::Light: break;
    }
  }
}

void RenderSystemImpl::doShadowPass()
{
  // TODO: Separate pass for every shadow-casting light
  const CRenderLight& firstLight =
    dynamic_cast<const CRenderLight&>(*m_components.at(*m_lights.begin()));
  const CSpatial& firstLightSpatial = m_spatialSystem.getComponent(firstLight.id());
  auto firstLightTransform = firstLightSpatial.absTransform();
  auto firstLightPos = getTranslation(firstLightTransform);
  auto firstLightDir = getDirection(firstLightTransform);
  auto firstLightMatrix = lookAt(firstLightPos, firstLightPos + firstLightDir);

  auto frustum = computeOrthographicFrustumPerimeter(firstLightPos, firstLightDir,
    degreesToRadians(90.f), firstLight.zFar);
  auto visible = m_spatialSystem.getIntersecting(frustum);

  m_renderer.beginPass(RenderPass::Shadow, firstLightPos, firstLightMatrix);

  drawEntities(visible, [](const Submodel& x) {
    return x.mesh.features.flags.test(MeshFeatures::CastsShadow);
  });

  m_renderer.endPass();
}

void RenderSystemImpl::doMainPass()
{
  auto frustum = computePerspectiveFrustumPerimeter(m_camera.getPosition(), m_camera.getDirection(),
    m_renderer.getViewParams().hFov);
  auto visible = m_spatialSystem.getIntersecting(frustum);

  m_renderer.beginPass(RenderPass::Main, m_camera.getPosition(), m_camera.getMatrix());

  drawEntities(visible);

  for (EntityId id : m_lights) {
    const CRenderLight& light = dynamic_cast<const CRenderLight&>(*m_components.at(id));
    const auto& spatial = m_spatialSystem.getComponent(id);
    const auto& transform = spatial.absTransform();

    m_renderer.drawLight(light.colour, light.ambient, light.specular, light.zFar, transform);

    if (light.submodels.size() > 0) {
      for (auto& submodel : light.submodels) {
        m_renderer.drawModel(submodel.mesh, submodel.material, spatial.absTransform());
      }
    }
  }

  m_renderer.endPass();
}

Mat4x4f interpolate(const Transform& A, const Transform& B, float_t t)
{
  assert(A.type == B.type);

  std::cout << "Interpolate, type = " << static_cast<int>(A.type) << "\n";
  std::cout << "A: " << A.data << "\n";
  std::cout << "B: " << B.data << "\n";
  std::cout << "t = " << t << "\n";

  Vec4f C = A.data + (B.data - A.data) * t;
  std::cout << "C: " << C << "\n";

  switch (A.type) {
    case TransformType::Rotation: return rotationMatrix4x4(Vec4f{ C[3], C[0], C[1], C[2] });
    case TransformType::Translation: return translationMatrix4x4(C.sub<3>());
    case TransformType::Scale: return scaleMatrix<float_t, 4>(C.sub<3>(), true);
  }
}

void computeAbsoluteJointTransforms(std::vector<Joint>& joints, const Mat4x4f& parentTransform,
  size_t index)
{
  auto& joint = joints[index];
  joint.absTransform = parentTransform * joint.transform;
  for (auto child : joint.children) {
    computeAbsoluteJointTransforms(joints, joint.absTransform, child);
  }
}

void computeAbsoluteJointTransforms(std::vector<Joint>& joints)
{
  computeAbsoluteJointTransforms(joints, identityMatrix<float_t, 4>(), 0);
}

// TODO: Optimise
std::vector<Mat4x4f> computeJointTransforms(const Skeleton& skeleton, const Skin& skin,
  const Animation& animation, AnimationState& state)
{
  Skeleton pose = skeleton;

  if (state.channels.empty()) {
    state.channels.resize(animation.channels.size());
  }

  for (size_t i = 0; i < animation.channels.size(); ++i) {
    auto& channel = animation.channels[i];
    if (state.channels[i].stopped) {
      continue;
    }

    auto& joint = pose.joints[skin.joints[channel.jointIndex]];

    size_t numFrames = channel.timestamps.size();
    float_t time = static_cast<float_t>(state.timer.elapsed());
    size_t& frame = state.channels[i].frame;

    assert(frame + 1 < numFrames);

    while (time >= channel.timestamps[frame + 1]) {
      ++frame;
      if (frame + 1 == numFrames) {
        frame = 0;
        state.channels[i].stopped = true;
        break;
      }
    }

    float_t frameDuration = channel.timestamps[frame + 1] - channel.timestamps[frame];
    float_t t = (time - channel.timestamps[frame]) / frameDuration;
    assert(t >= 0.f && t <= 1.f);

    const Transform& currentTransform = channel.transforms[frame];
    Transform prevTransform;
    prevTransform.type = channel.transforms[frame].type;
    if (frame > 1) {
      prevTransform.data = channel.transforms[frame - 1].data;
    }
    else {
      prevTransform.data = prevTransform.type == TransformType::Scale ?
        Vec4f{ 1.f, 1.f, 1.f, 0.f } :
        Vec4f{ 0.f, 0.f, 0.f, 0.f };
    }

    Mat4x4f transform = interpolate(prevTransform, currentTransform, t);
    joint.transform = joint.transform * transform;
  }

  computeAbsoluteJointTransforms(pose.joints);
  std::vector<Mat4x4f> absTransforms;
  for (auto& j : pose.joints) {
    absTransforms.push_back(j.absTransform);
  }

  std::cout << "Final transforms:\n";

  std::vector<Mat4x4f> finalTransforms;
  assert(skin.joints.size() == skin.inverseBindMatrices.size());
  for (size_t i = 0; i < skin.joints.size(); ++i) {
    finalTransforms.push_back(absTransforms[skin.joints[i]] * skin.inverseBindMatrices[i]);
    std::cout << finalTransforms.back() << "\n";
  }

  return finalTransforms;
}

void RenderSystemImpl::updateAnimations()
{
  for (auto& entry : m_animationStates) {
    auto& component = *m_components.at(entry.first);
    DBG_ASSERT(component.type == CRenderType::Model, "Can only play animation on models");

    auto& model = dynamic_cast<const CRenderModel&>(component);
    auto& state = entry.second;
    auto& animationSet = *m_animationSets.at(state.animationSet);
    auto& animation = *animationSet.animations.at(state.animationName); // TODO: Slow?

    for (auto& submodel : model.submodels) {
      auto& skeleton = *animationSet.skeleton;
      auto joints = computeJointTransforms(skeleton, *submodel.skin, animation, state); // TODO: Don't modify state for every submodel
      m_renderer.updateJointTransforms(submodel.mesh.id, joints);
    }
  }
}

// TODO: Hot path. Optimise
void RenderSystemImpl::update()
{
  try {
    updateAnimations();

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
