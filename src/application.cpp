#include "scene.hpp"
#include "logger.hpp"
#include "game.hpp"
#include "renderer.hpp"
#include "render_system.hpp"
#include "spatial_system.hpp"
#include "collision_system.hpp"
#include "map_parser.hpp"
#include "entity_factory.hpp"
#include "time.hpp"
#include "utils.hpp"
#include "units.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

namespace
{

class Application
{
  public:
    Application(GLFWwindow* window);

    void run();
    void onKeyboardInput(int key, int action);
    void onMouseMove(float_t x, float_t y);
    void onMouseClick();

  private:
    static Application* m_instance;
    static void onKeyboardInput(GLFWwindow* window, int code, int, int action, int);
    static void onMouseMove(GLFWwindow*, double x, double y);
    static void onMouseClick(GLFWwindow* window, int, int, int);

    GLFWwindow* m_window;
    LoggerPtr m_logger;
    RendererPtr m_renderer;
    SpatialSystemPtr m_spatialSystem;
    RenderSystemPtr m_renderSystem;
    CollisionSystemPtr m_collisionSystem;
    MapParserPtr m_mapParser;
    EntityFactoryPtr m_entityFactory;
    GamePtr m_game;

    Vec2f m_lastMousePos;

    void enterInputCapture();
    void exitInputCapture();
};

Application* Application::m_instance = nullptr;

void Application::onKeyboardInput(GLFWwindow*, int key, int, int action, int)
{
  if (m_instance) {
    m_instance->onKeyboardInput(key, action);
  }
}

void Application::onMouseMove(GLFWwindow*, double x, double y)
{
  if (m_instance) {
    m_instance->onMouseMove(static_cast<float_t>(x), static_cast<float_t>(y));
  }
}

void Application::onMouseClick(GLFWwindow*, int, int, int)
{
  if (m_instance) {
    m_instance->onMouseClick();
  }
}

Application::Application(GLFWwindow* window)
  : m_window(window)
{
  m_instance = this;

  m_logger = createLogger(std::cerr, std::cerr, std::cout, std::cout);
  m_renderer = createRenderer(*m_window, *m_logger);
  m_spatialSystem = createSpatialSystem(*m_logger);
  m_renderSystem = createRenderSystem(*m_spatialSystem, *m_renderer, *m_logger);
  m_collisionSystem = createCollisionSystem(*m_spatialSystem, *m_logger);
  m_mapParser = createMapParser(*m_logger);
  m_entityFactory = createEntityFactory(*m_spatialSystem, *m_renderSystem, *m_collisionSystem,
    *m_logger);

  auto player = createScene(*m_entityFactory, *m_spatialSystem, *m_renderSystem, *m_collisionSystem,
    *m_mapParser, *m_logger);

  m_game = createGame(std::move(player), *m_collisionSystem, *m_logger);

  glfwSetMouseButtonCallback(m_window, onMouseClick);
}

void Application::run()
{
  FrameRateLimiter frameRateLimiter(TARGET_FRAME_RATE);

  while(!glfwWindowShouldClose(m_window)) {
    glfwPollEvents();

    m_game->update();
    m_spatialSystem->update();
    m_renderSystem->update();
    m_collisionSystem->update();

    frameRateLimiter.wait();
  }
}

void Application::onKeyboardInput(int code, int action)
{
  auto key = static_cast<KeyboardKey>(code);

  if (key == KeyboardKey::Escape) {
    exitInputCapture();
  }

  if (action == GLFW_PRESS) {
    m_game->onKeyDown(key);
  }
  else if (action == GLFW_RELEASE) {
    m_game->onKeyUp(key);
  }
}

void Application::onMouseMove(float_t x, float_t y)
{
  Vec2f delta = (Vec2f{x, y} - m_lastMousePos) / Vec2f{WIDTH , HEIGHT};
  m_game->onMouseMove(delta);
  m_lastMousePos = { x, y };
}

void Application::onMouseClick()
{
  enterInputCapture();
}

void Application::enterInputCapture()
{
  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetKeyCallback(m_window, Application::onKeyboardInput);
  glfwSetCursorPosCallback(m_window, Application::onMouseMove);

  double x = 0;
  double y = 0;
  glfwGetCursorPos(m_window, &x, &y);
  m_lastMousePos = { static_cast<float_t>(x), static_cast<float_t>(y) };
}

void Application::exitInputCapture()
{
  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  glfwSetKeyCallback(m_window, nullptr);
  glfwSetCursorPosCallback(m_window, nullptr);
}

} // namespace

int main()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  std::string title = versionString();
  auto window = glfwCreateWindow(WIDTH, HEIGHT, title.c_str(), nullptr, nullptr);

  try {
    Application app(window);
    app.run();
  }
  catch(const std::exception& e) {
    std::cerr << e.what();
    return EXIT_FAILURE;
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
