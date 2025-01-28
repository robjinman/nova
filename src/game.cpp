#include "game.hpp"
#include "renderer.hpp"
#include "time.hpp"
#include "camera.hpp"
#include <set>

namespace
{

class GameImpl : public Game
{
  public:
    GameImpl(Renderer& renderer, Logger& logger);

    void onKeyDown(KeyboardKey key) override;
    void onKeyUp(KeyboardKey key) override;
    void update(const Vec2f& mouseDelta) override;

  private:
    Logger& m_logger;
    Renderer& m_renderer;
    Camera m_camera;
    std::set<KeyboardKey> m_keysPressed;
    Timer m_timer;
    ModelId m_model1Id;
    ModelId m_model2Id;
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

  m_camera.translate(Vec3f(0, 0, 8));
}

void GameImpl::onKeyDown(KeyboardKey key)
{
  m_keysPressed.insert(key);
}

void GameImpl::onKeyUp(KeyboardKey key)
{
  m_keysPressed.erase(key);
}

void GameImpl::update(const Vec2f& mouseDelta)
{
  const Mat4x4f model1Translation = glm::translate(Mat4x4f(1), Vec3f(-1.5, 0, 0));
  const Mat4x4f model2Translation = glm::translate(Mat4x4f(1), Vec3f(1.5, 0, 0));

  m_renderer.setModelTransform(m_model1Id, model1Translation * glm::rotate(Mat4x4f(1),
    glm::radians<float>((90.f * m_timer.elapsed())), Vec3f(0.f, 1.f, 0.f)));

  m_renderer.setModelTransform(m_model2Id, model2Translation * glm::rotate(Mat4x4f(1),
    glm::radians<float>((-90.f * m_timer.elapsed())), Vec3f(0.f, 1.f, 0.f)));

  m_renderer.update(m_camera);
}

} // namespace

GamePtr createGame(Renderer& renderer, Logger& logger)
{
  return std::make_unique<GameImpl>(renderer, logger);
}
