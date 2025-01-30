#include "game.hpp"
#include "renderer.hpp"
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
    GameImpl(Renderer& renderer, Logger& logger);

    void onKeyDown(KeyboardKey key) override;
    void onKeyUp(KeyboardKey key) override;
    void onMouseMove(const Vec2f& delta) override;
    void update() override;

  private:
    Logger& m_logger;
    Renderer& m_renderer;
    Camera m_camera;
    std::set<KeyboardKey> m_keysPressed;
    Vec2f m_mouseDelta;
    Timer m_timer;
    ModelId m_model1Id;
    ModelId m_model2Id;

    void updateModels();
    void handleKeyboardInput();
    void handleMouseInput();
};

GameImpl::GameImpl(Renderer& renderer, Logger& logger)
  : m_logger(logger)
  , m_renderer(renderer)
{
  auto texture1 = loadTexture("data/textures/texture1.png");
  auto texture1Id = m_renderer.addTexture(std::move(texture1));

  auto texture2 = loadTexture("data/textures/texture2.png");
  auto texture2Id = m_renderer.addTexture(std::move(texture2));

  auto model1 = loadModel("data/models/monkey.obj");
  model1->texture = texture1Id;
  auto model2 = loadModel("data/models/monkey.obj");
  model2->texture = texture2Id;

  m_model1Id = m_renderer.addModel(std::move(model1));
  m_model2Id = m_renderer.addModel(std::move(model2));

  m_camera.translate(Vec3f{0, 0, 8});
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
  m_renderer.setModelTransform(m_model1Id, transform(Vec3f{-1.5, 0, 0}, Vec3f{0, -angle, 0}));
  m_renderer.setModelTransform(m_model2Id, transform(Vec3f{1.5, 0, 0}, Vec3f{0, angle, 0}));
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

  dir[1] = 0;

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

  m_renderer.update(m_camera);
}

} // namespace

GamePtr createGame(Renderer& renderer, Logger& logger)
{
  return std::make_unique<GameImpl>(renderer, logger);
}
