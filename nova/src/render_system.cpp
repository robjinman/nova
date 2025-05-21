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
  size_t channelsComplete = 0;

  bool finished()
  {
    return channelsComplete == channels.size();
  }
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
  auto handle = m_renderer.addMesh(std::move(mesh));


  return handle;
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

    auto& component = *entry->second;
    const auto& spatial = m_spatialSystem.getComponent(id);

    switch(component.type) {
      case CRenderType::Model: {
        auto& model = dynamic_cast<CRenderModel&>(component);
        for (auto& submodel : model.submodels) {
          if (filter(submodel)) {
            if (model.isInstanced) {
              m_renderer.drawInstance(submodel.mesh, submodel.material, spatial.absTransform());
            }
            else {
              if (submodel.jointTransformsDirty) {
                m_renderer.drawModel(submodel.mesh, submodel.material,
                  spatial.absTransform() * submodel.mesh.transform, submodel.jointTransforms);
                submodel.jointTransformsDirty = false;
              }
              else {
                m_renderer.drawModel(submodel.mesh, submodel.material,
                  spatial.absTransform() * submodel.mesh.transform);
              }
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

Vec4f interpolateRotation(const Vec4f& A_, const Vec4f& B_, float_t t)
{
  Vec4f A = A_;
  Vec4f B = B_;

  float_t dotProduct = A.dot(B);
  if (dotProduct < 0.f) {
    B = -B;
    dotProduct = -dotProduct;
  }
  
  if (dotProduct > 0.9955f) {
    return (A + (B - A) * t).normalise();
  }

  float_t theta0 = acos(dotProduct);
  float_t theta = t * theta0;
  float_t sinTheta = sin(theta);
  float_t sinTheta0 = sin(theta0);

  float_t scaleA = cos(theta) - dotProduct * sinTheta / sinTheta0;
  float_t scaleB = sinTheta / sinTheta0;

  return A * scaleA + B * scaleB;
}

Vec3f interpolateTranslation(const Vec3f& A, const Vec3f& B, float_t t)
{
  return A + (B - A) * t;
}

Vec3f interpolateScale(const Vec3f& A, const Vec3f& B, float_t t)
{
  return A + (B - A) * t;
}

Transform interpolate(const Transform& A, const Transform& B, float_t t)
{
  Transform C;
  if (!A.rotation.has_value()) {
    C.rotation = B.rotation;
  }
  else if (!B.rotation.has_value()) {
    C.rotation = A.rotation;
  }
  else if (A.rotation.has_value() && B.rotation.has_value()) {
    C.rotation = interpolateRotation(A.rotation.value(), B.rotation.value(), t);
  }
  if (!A.translation.has_value()) {
    C.translation = B.translation;
  }
  else if (!B.translation.has_value()) {
    C.translation = A.translation;
  }
  else if (A.translation.has_value() && B.translation.has_value()) {
    C.translation = interpolateTranslation(A.translation.value(), B.translation.value(), t);
  }
  if (!A.scale.has_value()) {
    C.scale = B.scale;
  }
  else if (!B.scale.has_value()) {
    C.scale = A.scale;
  }
  else if (A.scale.has_value() && B.scale.has_value()) {
    C.scale = interpolateScale(A.scale.value(), B.scale.value(), t);
  }
  return C;
}

struct PosedJoint
{
  Transform transform;
  std::vector<size_t> children;
};

void computeAbsoluteJointTransforms(const std::vector<PosedJoint>& pose,
  std::vector<Mat4x4f>& absTransforms, const Mat4x4f& parentTransform, size_t index)
{
  absTransforms[index] = parentTransform * pose[index].transform.toMatrix();
  for (size_t i : pose[index].children) {
    computeAbsoluteJointTransforms(pose, absTransforms, absTransforms[index], i);
  }
}

// TODO: Optimise
std::vector<Mat4x4f> computeJointTransforms(const Skeleton& skeleton, const Skin& skin,
  const Animation& animation, AnimationState& state)
{
  std::vector<PosedJoint> pose(skeleton.joints.size());
  for (size_t i = 0; i < skeleton.joints.size(); ++i) {
    pose[i].children = skeleton.joints[i].children;
  }
  pose[skeleton.rootNodeIndex].transform = skeleton.joints[skeleton.rootNodeIndex].transform;

  if (state.channels.empty()) {
    state.channels.resize(animation.channels.size());
  }

  // Returns true if channel is finished
  auto advanceFrame = [](float_t time, const AnimationChannel& channel, size_t& frame,
    size_t numFrames) {

    // Loop until the next frame is in the future
    while (channel.timestamps[frame + 1] <= time) {
      ++frame;
      if (frame + 1 == numFrames) {
        return true;
      }
    }
    return false;
  };

  for (size_t i = 0; i < animation.channels.size(); ++i) {
    auto& channel = animation.channels[i];
    size_t& frame = state.channels[i].frame;
    auto& joint = pose[channel.jointIndex];

    if (state.channels[i].stopped) {
      joint.transform.mix(channel.transforms[frame]);
      continue;
    }

    size_t numFrames = channel.timestamps.size();
    float_t time = static_cast<float_t>(state.timer.elapsed());

    assert(frame + 1 < numFrames);

    if (advanceFrame(time, channel, frame, numFrames)) {
      state.channels[i].stopped = true;
      ++state.channelsComplete;
      joint.transform.mix(channel.transforms[frame]);
      continue;
    }
    else if (frame == 0 && time < channel.timestamps[0]) {
      joint.transform.mix(channel.transforms[frame]);
      continue;
    }

    float_t frameDuration = channel.timestamps[frame + 1] - channel.timestamps[frame];
    float_t t = (time - channel.timestamps[frame]) / frameDuration;
    assert(t >= 0.f && t <= 1.f);

    const Transform& prevTransform = channel.transforms[frame];
    const Transform& nextTransform = channel.transforms[frame + 1];

    joint.transform.mix(interpolate(prevTransform, nextTransform, t));
  }

  std::vector<Mat4x4f> absTransforms(skeleton.joints.size());
  computeAbsoluteJointTransforms(pose, absTransforms, identityMatrix<float_t, 4>(),
    skeleton.rootNodeIndex);

  std::vector<Mat4x4f> finalTransforms;
  assert(skin.joints.size() == skin.inverseBindMatrices.size());
  for (size_t i = 0; i < skin.joints.size(); ++i) {
    finalTransforms.push_back(absTransforms[skin.joints[i]] * skin.inverseBindMatrices[i]);
  }

  return finalTransforms;
}

void RenderSystemImpl::updateAnimations()
{
  for (auto i = m_animationStates.begin(); i != m_animationStates.end();) {
    auto& component = *m_components.at(i->first);
    DBG_ASSERT(component.type == CRenderType::Model, "Can only play animation on models");

    auto& model = dynamic_cast<CRenderModel&>(component);
    auto& state = i->second;
    auto& animationSet = *m_animationSets.at(state.animationSet);
    auto& animation = *animationSet.animations.at(state.animationName); // TODO: Slow?

    for (auto& submodel : model.submodels) {
      auto& skeleton = *animationSet.skeleton;
      submodel.jointTransforms = computeJointTransforms(skeleton, *submodel.skin, animation, state);
      submodel.jointTransformsDirty = true;
    }

    if (state.finished()) {
      i = m_animationStates.erase(i++);
    }
    else {
      ++i;
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
