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
#include "window_delegate.hpp"
#include "file_system.hpp"
#include <iostream>
#include <GLFW/glfw3.h>

const int RESOLUTION_W = 1920;
const int RESOLUTION_H = 1080;

WindowDelegatePtr createWindowDelegate(GLFWwindow& window);
FileSystemPtr createDefaultFileSystem(const std::filesystem::path& dataRootDir);

namespace
{

struct WindowState
{
  int posX = 0;
  int posY = 0;
  int width = 0;
  int height = 0;
};

class Application
{
  public:
    Application();

    void run();
    void onKeyboardInput(int key, int action);
    void onMouseMove(float_t x, float_t y);
    void onMouseClick();

    ~Application();

  private:
    static Application* m_instance;
    static void onKeyboardInput(GLFWwindow* window, int code, int, int action, int);
    static void onMouseMove(GLFWwindow*, double x, double y);
    static void onMouseClick(GLFWwindow* window, int, int, int);

    GLFWwindow* m_window;
    FileSystemPtr m_fileSystem;
    WindowDelegatePtr m_windowDelegate;
    LoggerPtr m_logger;
    RendererPtr m_renderer;
    SpatialSystemPtr m_spatialSystem;
    RenderSystemPtr m_renderSystem;
    CollisionSystemPtr m_collisionSystem;
    MapParserPtr m_mapParser;
    EntityFactoryPtr m_entityFactory;
    GamePtr m_game;

    bool m_fullscreen = false;
    WindowState m_initialWindowState;
    Vec2f m_lastMousePos;

    void enterInputCapture();
    void exitInputCapture();
    void toggleFullScreen();
    Vec2i windowSize() const;
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

Application::Application()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  m_instance = this;

  std::string title = versionString();
  m_window = glfwCreateWindow(RESOLUTION_W, RESOLUTION_H, title.c_str(), nullptr, nullptr);
  glfwGetWindowPos(m_window, &m_initialWindowState.posX, &m_initialWindowState.posY);
  glfwGetWindowSize(m_window, &m_initialWindowState.width, &m_initialWindowState.height);

  m_fileSystem = createDefaultFileSystem(std::filesystem::current_path() / "data");
  m_windowDelegate = createWindowDelegate(*m_window);
  m_logger = createLogger(std::cerr, std::cerr, std::cout, std::cout);
  m_renderer = createRenderer(*m_fileSystem, *m_windowDelegate, *m_logger);
  m_spatialSystem = createSpatialSystem(*m_logger);
  m_renderSystem = createRenderSystem(*m_spatialSystem, *m_renderer, *m_logger);
  m_collisionSystem = createCollisionSystem(*m_spatialSystem, *m_logger);
  m_mapParser = createMapParser(*m_fileSystem, *m_logger);
  m_entityFactory = createEntityFactory(*m_spatialSystem, *m_renderSystem, *m_collisionSystem,
    *m_fileSystem, *m_logger);

  auto player = createScene(*m_entityFactory, *m_spatialSystem, *m_renderSystem, *m_collisionSystem,
    *m_mapParser, *m_fileSystem, *m_logger);

  m_renderSystem->start();
  m_game = createGame(std::move(player), *m_collisionSystem, *m_logger);

  glfwSetMouseButtonCallback(m_window, onMouseClick);
}

void Application::run()
{
  FrameRateLimiter frameRateLimiter{TARGET_FRAME_RATE};

  while(!glfwWindowShouldClose(m_window)) {
    glfwPollEvents();

    m_game->update();
    m_spatialSystem->update();
    m_renderSystem->update();
    m_collisionSystem->update();

    frameRateLimiter.wait();
  }
}

Vec2i Application::windowSize() const
{
  Vec2i size;
  glfwGetWindowSize(m_window, &size[0], &size[1]);
  return size;
}

void Application::onKeyboardInput(int code, int action)
{
  auto key = static_cast<KeyboardKey>(code);

  if (action == GLFW_PRESS) {
    m_game->onKeyDown(key);

    switch (key) {
      case KeyboardKey::Escape:
        exitInputCapture();
        break;
      case KeyboardKey::F:
        m_logger->info(STR("Renderer frame rate: " << m_renderer->frameRate()));
        break;
      case KeyboardKey::F11:
        toggleFullScreen();
        break;
      default: break;
    }
  }
  else if (action == GLFW_RELEASE) {
    m_game->onKeyUp(key);
  }
}

void Application::toggleFullScreen()
{
  if (m_fullscreen) {
    glfwSetWindowMonitor(m_window, NULL, m_initialWindowState.posX, m_initialWindowState.posY,
      m_initialWindowState.width, m_initialWindowState.height, 0);

    m_renderer->onResize();
    m_fullscreen = false;
  }
  else {
    glfwGetWindowPos(m_window, &m_initialWindowState.posX, &m_initialWindowState.posY);
    glfwGetWindowSize(m_window, &m_initialWindowState.width, &m_initialWindowState.height);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    glfwSetWindowMonitor(m_window, monitor, 0, 0, RESOLUTION_W, RESOLUTION_H, mode->refreshRate);

    m_renderer->onResize();
    m_fullscreen = true;
  }
}

void Application::onMouseMove(float_t x, float_t y)
{
  Vec2f delta = (Vec2f{x, y} - m_lastMousePos) / static_cast<Vec2f>(windowSize());
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

Application::~Application()
{
  m_renderer.reset();
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

} // namespace

int main()
{
  try {
    Application app;
    app.run();
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
