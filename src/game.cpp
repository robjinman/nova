#include "game.hpp"
#include "render_system.hpp"
#include "spatial_system.hpp"
#include "collision_system.hpp"
#include "time.hpp"
#include "camera.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include <set>

namespace
{

const float_t MOUSE_LOOK_SPEED = 2.5;
const float_t PLAYER_SPEED = 0.5f;

class GameImpl : public Game
{
  public:
    GameImpl(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
      CollisionSystem& collisionSystem, Logger& logger);

    void onKeyDown(KeyboardKey key) override;
    void onKeyUp(KeyboardKey key) override;
    void onMouseMove(const Vec2f& delta) override;
    void update() override;

  private:
    Logger& m_logger;
    SpatialSystem& m_spatialSystem;
    RenderSystem& m_renderSystem;
    CollisionSystem& m_collisionSystem;
    Camera& m_camera;
    std::set<KeyboardKey> m_keysPressed;
    Vec2f m_mouseDelta;
    Timer m_timer;
    std::vector<EntityId> m_model1Entities;
    std::vector<EntityId> m_model2Entities;

    void updateModels();
    void handleKeyboardInput();
    void handleMouseInput();
};

GameImpl::GameImpl(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, Logger& logger)
  : m_logger(logger)
  , m_spatialSystem(spatialSystem)
  , m_renderSystem(renderSystem)
  , m_collisionSystem(collisionSystem)
  , m_camera(m_renderSystem.camera())
{
/*
  const size_t nModel1s = 10;
  const size_t nModel2s = 10;

  auto texture1 = loadTexture("data/textures/texture1.png");
  auto texture2 = loadTexture("data/textures/texture2.png");
  auto texture1Id = m_renderSystem.addTexture(std::move(texture1));
  auto texture2Id = m_renderSystem.addTexture(std::move(texture2));
  auto mesh1 = loadMesh("data/models/monkey.obj");
  auto mesh2 = loadMesh("data/models/monkey.obj");
  mesh2->isInstanced = true;
  mesh2->maxInstances = nModel2s;
  auto mesh1Id = m_renderSystem.addMesh(std::move(mesh1));
  auto mesh2Id = m_renderSystem.addMesh(std::move(mesh2));
  auto material1 = std::make_unique<Material>();
  material1->texture = texture1Id;
  auto material2 = std::make_unique<Material>();
  material2->texture = texture2Id;
  auto material1Id = m_renderSystem.addMaterial(std::move(material1));
  auto material2Id = m_renderSystem.addMaterial(std::move(material2));

  for (size_t i = 0; i < nModel1s; ++i) {
    auto entityId = System::nextId();

    auto spatialComp = std::make_unique<CSpatial>(entityId);
    m_spatialSystem.addComponent(std::move(spatialComp));

    auto renderComp = std::make_unique<CRender>(entityId);
    renderComp->material = material1Id;
    renderComp->mesh = mesh1Id;
    m_renderSystem.addComponent(std::move(renderComp));

    m_model1Entities.push_back(entityId);
  }

  for (size_t i = 0; i < nModel2s; ++i) {
    auto entityId = System::nextId();

    auto spatialComp = std::make_unique<CSpatial>(entityId);
    m_spatialSystem.addComponent(std::move(spatialComp));

    auto renderComp = std::make_unique<CRender>(entityId);
    renderComp->material = material2Id;
    renderComp->mesh = mesh2Id;
    m_renderSystem.addComponent(std::move(renderComp));

    m_model2Entities.push_back(entityId);
  }

  m_camera.translate(Vec3f{0, 0, 8});*/
}

void GameImpl::onKeyDown(KeyboardKey key)
{
  m_keysPressed.insert(key);
}

void GameImpl::onKeyUp(KeyboardKey key)
{
  m_keysPressed.erase(key);
}

void GameImpl::onMouseMove(const Vec2f& delta)
{
  m_mouseDelta = delta;
}

void GameImpl::updateModels()
{
  float_t angle = degreesToRadians<float_t>(90.f * m_timer.elapsed());
  for (size_t i = 0; i < m_model1Entities.size(); ++i) {
    auto& spatial = m_spatialSystem.getComponent(m_model1Entities[i]);
    spatial.setTransform(transform(Vec3f{-1.5, 0, -3.f * i}, Vec3f{0, -angle, 0}));
  }
  for (size_t i = 0; i < m_model2Entities.size(); ++i) {
    auto& spatial = m_spatialSystem.getComponent(m_model2Entities[i]);
    spatial.setTransform(transform(Vec3f{1.5, 0, -3.f * i}, Vec3f{0, angle, 0}));
  }
}

void GameImpl::handleKeyboardInput()
{
  Vec3f dir{};

  if (m_keysPressed.count(KeyboardKey::W)) {
    dir = m_camera.getDirection();
  }
  if (m_keysPressed.count(KeyboardKey::S)) {
    dir = -m_camera.getDirection();
  }
  if (m_keysPressed.count(KeyboardKey::D)) {
    dir = m_camera.getDirection().cross(Vec3f{0, 1, 0});
  }
  if (m_keysPressed.count(KeyboardKey::A)) {
    dir = -m_camera.getDirection().cross(Vec3f{0, 1, 0});
  }

  //dir[1] = 0;

  if (dir != Vec3f{}) {
    dir = dir.normalise();
    m_camera.translate(dir * PLAYER_SPEED);
  }
}

void GameImpl::handleMouseInput()
{
  m_camera.rotate(-MOUSE_LOOK_SPEED * m_mouseDelta[1], -MOUSE_LOOK_SPEED * m_mouseDelta[0]);
  m_mouseDelta = Vec2f{};
}

void GameImpl::update()
{
  handleKeyboardInput();
  handleMouseInput();
  updateModels();
}

} // namespace

GamePtr createGame(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, Logger& logger)
{
  return std::make_unique<GameImpl>(spatialSystem, renderSystem, collisionSystem, logger);
}
