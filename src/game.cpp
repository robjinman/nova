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
const float_t PLAYER_SPEED = 1.f;

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
}

} // namespace

GamePtr createGame(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, Logger& logger)
{
  return std::make_unique<GameImpl>(spatialSystem, renderSystem, collisionSystem, logger);
}
