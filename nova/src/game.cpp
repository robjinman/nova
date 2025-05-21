#include "game.hpp"
#include "player.hpp"
#include "render_system.hpp"
#include "collision_system.hpp"
#include "time.hpp"
#include "camera.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "units.hpp"
#include <set>
#undef max
#undef min

namespace
{

const float_t GRAVITY_STRENGTH = 3.5f;
const float_t BUOYANCY = 0.1f;
const float_t MOUSE_LOOK_SPEED = 2.5f;

KeyboardKey buttonToKey(GamepadButton button)
{
  switch (button) {
    case GamepadButton::A: return KeyboardKey::E;
    case GamepadButton::Y: return KeyboardKey::F;
    // ...
    default: return KeyboardKey::Unknown;
  }
}

class GameImpl : public Game
{
  public:
    GameImpl(PlayerPtr player, RenderSystem& renderSystem, CollisionSystem& collisionSystem,
      Logger& logger);

    void onKeyDown(KeyboardKey key) override;
    void onKeyUp(KeyboardKey key) override;
    void onButtonDown(GamepadButton button) override;
    void onButtonUp(GamepadButton button) override;
    void onMouseMove(const Vec2f& delta) override;
    void onLeftStickMove(const Vec2f& delta) override;
    void onRightStickMove(const Vec2f& delta) override;
    void update() override;

  private:
    Logger& m_logger;
    RenderSystem& m_renderSystem;
    CollisionSystem& m_collisionSystem;
    PlayerPtr m_player;
    std::set<KeyboardKey> m_keysPressed;
    Vec2f m_mouseDelta;
    Vec2f m_leftStickDelta;
    Timer m_timer;
    size_t m_frame = 0;
    double m_measuredFrameRate = 0;
    float_t m_playerVerticalVelocity = 0;  // World units per frame
    bool m_freeflyMode = false;

    void measureFrameRate();
    void processKeyboardInput();
    void processMouseInput();
    void gravity();
    float_t g() const; // World units per frame per frame
};

GameImpl::GameImpl(PlayerPtr player, RenderSystem& renderSystem, CollisionSystem& collisionSystem,
  Logger& logger)
  : m_logger(logger)
  , m_renderSystem(renderSystem)
  , m_collisionSystem(collisionSystem)
  , m_player(std::move(player))
{
}

float_t GameImpl::g() const
{
  return GRAVITY_STRENGTH * metresToWorldUnits(9.8f) / (TARGET_FRAME_RATE * TARGET_FRAME_RATE);
}

void GameImpl::onKeyDown(KeyboardKey key)
{
  m_keysPressed.insert(key);

  switch (key) {
    case KeyboardKey::F:{
      m_logger.info(STR("Simulation frame rate: " << m_measuredFrameRate));
      break;
    }
    case KeyboardKey::P: {
      m_freeflyMode = !m_freeflyMode;
      m_logger.info(STR("Freefly mode: " << (m_freeflyMode ? "ON" : "OFF")));
      break;
    }
    case KeyboardKey::R: {
      m_logger.info("Playing 'bend' animation on 'richard' entity");
      EntityId richard = System::idFromString("richard");
      m_renderSystem.playAnimation(richard, "bend");
      break;
    }
    case KeyboardKey::J: {
      m_logger.info("Playing 'Attack' animation on 'james' entity");
      EntityId james = System::idFromString("james");
      m_renderSystem.playAnimation(james, "Attack");
      break;
    }
    case KeyboardKey::K: {
      m_logger.info("Playing 'Idle' animation on 'james' entity");
      EntityId james = System::idFromString("james");
      m_renderSystem.playAnimation(james, "Idle");
      break;
    }
    case KeyboardKey::L: {
      m_logger.info("Playing 'Walk' animation on 'james' entity");
      EntityId james = System::idFromString("james");
      m_renderSystem.playAnimation(james, "Walk");
      break;
    }
    default: break;
  }
}

void GameImpl::onKeyUp(KeyboardKey key)
{
  m_keysPressed.erase(key);
}

void GameImpl::onButtonDown(GamepadButton button)
{
  onKeyDown(buttonToKey(button));
}

void GameImpl::onButtonUp(GamepadButton button)
{
  onKeyUp(buttonToKey(button));
}

void GameImpl::onMouseMove(const Vec2f& delta)
{
  m_mouseDelta = delta;
}

void GameImpl::onLeftStickMove(const Vec2f& delta)
{
  m_leftStickDelta = delta;
}

void GameImpl::onRightStickMove(const Vec2f& delta)
{
  float_t epsilon = 0.15f;
  float_t sensitivity = 0.025f;
  m_mouseDelta = {
    fabs(delta[0]) > epsilon ? sensitivity * delta[0] : 0.f,
    fabs(delta[1]) > epsilon ? sensitivity * delta[1] : 0.f
  };
}

void GameImpl::gravity()
{
  m_player->translate(Vec3f{ 0, m_playerVerticalVelocity, 0 });
  float_t altitude = m_collisionSystem.altitude(m_player->getPosition());

  if (altitude > 0) {
    m_playerVerticalVelocity = std::max(m_playerVerticalVelocity - g(), -altitude);
  }
  else if (altitude < 0) {
    m_playerVerticalVelocity = clip(m_playerVerticalVelocity - BUOYANCY * altitude, 0.f, -altitude);
  }
  else if (altitude == 0) {
    m_playerVerticalVelocity = 0;
  }
}

void GameImpl::processKeyboardInput()
{
  Vec3f direction{};
  float_t speed = m_player->getSpeed();
  const auto forward = m_player->getDirection();
  const auto strafe = m_player->getDirection().cross(Vec3f{0, 1, 0});

  if (m_leftStickDelta != Vec2f{}) {
    float_t threshold = 0.4f;

    if (m_leftStickDelta.magnitude() > threshold) {
      float_t x = m_leftStickDelta[0];
      float_t y = -m_leftStickDelta[1];
      direction = forward * y + strafe * x;
      speed = m_leftStickDelta.magnitude() * m_player->getSpeed();
    }

    m_leftStickDelta = Vec2f{};
  }
  else {
    if (m_keysPressed.count(KeyboardKey::W)) {
      direction += forward;
    }
    if (m_keysPressed.count(KeyboardKey::S)) {
      direction -= forward;
    }
    if (m_keysPressed.count(KeyboardKey::D)) {
      direction += strafe;
    }
    if (m_keysPressed.count(KeyboardKey::A)) {
      direction -= strafe;
    }
  }

  if (m_keysPressed.count(KeyboardKey::E)) {
    if (m_collisionSystem.altitude(m_player->getPosition()) == 0) {
      m_playerVerticalVelocity = sqrt(m_player->getJumpHeight() * 2.f * g());
    }
  }
  
  if (direction != Vec3f{}) {
    if (!m_freeflyMode) {
      direction[1] = 0;
    }

    direction = direction.normalise();
    auto playerPos = m_player->getPosition();
    auto desiredDelta = direction * speed / static_cast<float_t>(TARGET_FRAME_RATE);

    Vec3f delta = m_collisionSystem.tryMove(playerPos, desiredDelta, m_player->getRadius(),
      m_player->getStepHeight());

    m_player->translate(delta);
  }
}

void GameImpl::processMouseInput()
{
  m_player->rotate(-MOUSE_LOOK_SPEED * m_mouseDelta[1], -MOUSE_LOOK_SPEED * m_mouseDelta[0]);
  m_mouseDelta = Vec2f{};
}

void GameImpl::measureFrameRate()
{
  ++m_frame;
  if (m_frame % TARGET_FRAME_RATE == 0) {
    m_measuredFrameRate = TARGET_FRAME_RATE / m_timer.elapsed();
    m_timer.reset();
  }
}

void GameImpl::update()
{
  measureFrameRate();
  processKeyboardInput();
  processMouseInput();
  //m_player->rotate(0.f, 0.01f); // TODO: Remove
  if (!m_freeflyMode) {
    gravity();
  }
}

} // namespace

GamePtr createGame(PlayerPtr player, RenderSystem& renderSystem, CollisionSystem& collisionSystem,
  Logger& logger)
{
  return std::make_unique<GameImpl>(std::move(player), renderSystem, collisionSystem, logger);
}
