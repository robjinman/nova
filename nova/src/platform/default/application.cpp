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

const int WINDOWED_RESOLUTION_W = 800;
const int WINDOWED_RESOLUTION_H = 600;
const int FULLSCREEN_RESOLUTION_W = 1920;
const int FULLSCREEN_RESOLUTION_H = 1080;

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

enum class ControlMode
{
  KeyboardMouse,
  Gamepad
};

GamepadButton buttonCode(int button)
{
  switch (button) {
    case GLFW_GAMEPAD_BUTTON_A:             return GamepadButton::A;
    case GLFW_GAMEPAD_BUTTON_B:             return GamepadButton::B;
    case GLFW_GAMEPAD_BUTTON_X:             return GamepadButton::X;
    case GLFW_GAMEPAD_BUTTON_Y:             return GamepadButton::Y;
    case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER:   return GamepadButton::L1;
    case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER:  return GamepadButton::R1;
    // ...
    default:                                return GamepadButton::Unknown;
  }
}

class Application
{
  public:
    Application();

    void run();
    void onKeyboardInput(int key, int action);
    void onMouseMove(float_t x, float_t y);
    void onMouseClick();
    void onJoystickEvent(int event);

    ~Application();

  private:
    static Application* m_instance;
    static void onKeyboardInput(GLFWwindow* window, int code, int, int action, int);
    static void onMouseMove(GLFWwindow*, double x, double y);
    static void onMouseClick(GLFWwindow* window, int, int, int);
    static void onJoystickEvent(int, int event);

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
    ControlMode m_controlMode;
    Vec2f m_lastMousePos;
    GLFWgamepadstate m_gamepadState;

    void enterInputCapture();
    void exitInputCapture();
    void toggleFullScreen();
    Vec2i windowSize() const;
    void processGamepadInput();
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

void Application::onJoystickEvent(int, int event)
{
  if (m_instance) {
    m_instance->onJoystickEvent(event);
  }
}

Application::Application()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  m_instance = this;

  std::string title = versionString();
  m_window = glfwCreateWindow(WINDOWED_RESOLUTION_W, WINDOWED_RESOLUTION_H, title.c_str(),
    nullptr, nullptr);
  glfwGetWindowPos(m_window, &m_initialWindowState.posX, &m_initialWindowState.posY);
  glfwGetWindowSize(m_window, &m_initialWindowState.width, &m_initialWindowState.height);

  m_controlMode = glfwJoystickPresent(GLFW_JOYSTICK_1) ?
    ControlMode::Gamepad :
    ControlMode::KeyboardMouse;

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
    if (m_controlMode == ControlMode::Gamepad) {
      processGamepadInput();
    }

    frameRateLimiter.wait();
  }
}

Vec2i Application::windowSize() const
{
  Vec2i size;
  glfwGetWindowSize(m_window, &size[0], &size[1]);
  return size;
}

void Application::onJoystickEvent(int event)
{
  m_logger->info(STR("Received joystick event: " << event));

  switch (event) {
    case GLFW_CONNECTED:
      exitInputCapture();
      m_controlMode = ControlMode::Gamepad;
      enterInputCapture();
      break;
    case GLFW_DISCONNECTED:
      exitInputCapture();
      m_controlMode = ControlMode::KeyboardMouse;
      enterInputCapture();
      break;
  }
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
#ifdef __APPLE__
      case KeyboardKey::F12:
#else
      case KeyboardKey::F11:
#endif
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

    glfwSetWindowMonitor(m_window, monitor, 0, 0, FULLSCREEN_RESOLUTION_W, FULLSCREEN_RESOLUTION_H,
      mode->refreshRate);

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
  glfwSetJoystickCallback(Application::onJoystickEvent);

  double x = 0;
  double y = 0;
  glfwGetCursorPos(m_window, &x, &y);
  m_lastMousePos = { static_cast<float_t>(x), static_cast<float_t>(y) };
}

void Application::processGamepadInput()
{
  GLFWgamepadstate state;

  if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
    for (int i = 0; i < GLFW_GAMEPAD_BUTTON_LAST; ++i) {
      if (m_gamepadState.buttons[i] == GLFW_RELEASE && state.buttons[i] == GLFW_PRESS) {
        m_game->onButtonDown(buttonCode(i));
      }
      else if (m_gamepadState.buttons[i] == GLFW_PRESS && state.buttons[i] == GLFW_RELEASE) {
        m_game->onButtonUp(buttonCode(i));
      }
    }
  }

  float_t leftX = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
  float_t leftY = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
  m_game->onLeftStickMove({ leftX, leftY });

  float_t rightX = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
  float_t rightY = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
  m_game->onRightStickMove({ rightX, rightY });

  m_gamepadState = state;
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
