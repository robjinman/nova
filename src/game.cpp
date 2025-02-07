#include "game.hpp"
#include "player.hpp"
#include "collision_system.hpp"
#include "time.hpp"
#include "camera.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "units.hpp"
#include <set>

namespace
{

const float_t MOUSE_LOOK_SPEED = 2.5;

class GameImpl : public Game
{
  public:
    GameImpl(PlayerPtr player, CollisionSystem& collisionSystem, Logger& logger);

    void onKeyDown(KeyboardKey key) override;
    void onKeyUp(KeyboardKey key) override;
    void onMouseMove(const Vec2f& delta) override;
    void update() override;

  private:
    Logger& m_logger;
    CollisionSystem& m_collisionSystem;
    PlayerPtr m_player;
    std::set<KeyboardKey> m_keysPressed;
    Vec2f m_mouseDelta;
    Timer m_timer;
    size_t m_frame = 0;
    double m_measuredFrameRate = 0;

    void handleKeyboardInput();
    void handleMouseInput();
};

GameImpl::GameImpl(PlayerPtr player, CollisionSystem& collisionSystem, Logger& logger)
  : m_logger(logger)
  , m_collisionSystem(collisionSystem)
  , m_player(std::move(player))
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
    dir = m_player->getDirection();
  }
  if (m_keysPressed.count(KeyboardKey::S)) {
    dir = -m_player->getDirection();
  }
  if (m_keysPressed.count(KeyboardKey::D)) {
    dir = m_player->getDirection().cross(Vec3f{0, 1, 0});
  }
  if (m_keysPressed.count(KeyboardKey::A)) {
    dir = -m_player->getDirection().cross(Vec3f{0, 1, 0});
  }

  dir[1] = 0;

  if (dir != Vec3f{}) {
    dir = dir.normalise();
    m_player->translate(dir * m_player->getSpeed() / static_cast<float_t>(TARGET_FRAME_RATE));
  }

  if (m_keysPressed.count(KeyboardKey::F)) {
    m_logger.info(STR("Frame rate: " << m_measuredFrameRate));
  }
}

void GameImpl::handleMouseInput()
{
  m_player->rotate(-MOUSE_LOOK_SPEED * m_mouseDelta[1], -MOUSE_LOOK_SPEED * m_mouseDelta[0]);
  m_mouseDelta = Vec2f{};
}

void GameImpl::update()
{
  ++m_frame;
  if (m_frame % TARGET_FRAME_RATE == 0) {
    m_measuredFrameRate = TARGET_FRAME_RATE / m_timer.elapsed();
    m_timer.reset();
  }

  handleKeyboardInput();
  handleMouseInput();
}

} // namespace

GamePtr createGame(PlayerPtr player, CollisionSystem& collisionSystem, Logger& logger)
{
  return std::make_unique<GameImpl>(std::move(player), collisionSystem, logger);
}
