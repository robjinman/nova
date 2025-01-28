#include "version.hpp"
#include "logger.hpp"
#include "game.hpp"
#include "renderer.hpp"
#include "time.hpp"
#include "utils.hpp"
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
    void onMouseMove(double x, double y);
    void onMouseClick();

  private:
    static Application* m_instance;
    static void onKeyboardInput(GLFWwindow* window, int code, int, int action, int);
    static void onMouseMove(GLFWwindow*, double x, double y);
    static void onMouseClick(GLFWwindow* window, int, int, int);

    GLFWwindow* m_window;
    LoggerPtr m_logger;
    RendererPtr m_renderer;
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
    m_instance->onMouseMove(x, y);
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
  m_game = createGame(*m_renderer, *m_logger);

  glfwSetMouseButtonCallback(m_window, onMouseClick);
}

void Application::run()
{
  FrameRateLimiter frameRateLimiter(60);

  while(!glfwWindowShouldClose(m_window)) {
    glfwPollEvents();

    m_game->update();

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

void Application::onMouseMove(double x, double y)
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
  m_lastMousePos = { x, y };
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

  std::string title = STR("Nova " << Nova_VERSION_MAJOR << "." << Nova_VERSION_MINOR);
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
