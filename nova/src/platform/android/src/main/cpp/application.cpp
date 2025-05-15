#include "scene.hpp"
#include "logger.hpp"
#include "game.hpp"
#include "renderer.hpp"
#include "render_system.hpp"
#include "spatial_system.hpp"
#include "collision_system.hpp"
#include "map_parser.hpp"
#include "model_loader.hpp"
#include "entity_factory.hpp"
#include "time.hpp"
#include "utils.hpp"
#include "units.hpp"
#include "window_delegate.hpp"
#include "file_system.hpp"
#include "units.hpp"
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <iostream>

LoggerPtr createAndroidLogger();
WindowDelegatePtr createWindowDelegate(ANativeWindow& window);
FileSystemPtr createAndroidFileSystem(AAssetManager& assetManager);

namespace
{

GamepadButton buttonCode(int32_t key)
{
  switch (key) {
    case AKEYCODE_BUTTON_A:   return GamepadButton::A;
    case AKEYCODE_BUTTON_B:   return GamepadButton::B;
    case AKEYCODE_BUTTON_X:   return GamepadButton::X;
    case AKEYCODE_BUTTON_Y:   return GamepadButton::Y;
    case AKEYCODE_BUTTON_L1:  return GamepadButton::L1;
    case AKEYCODE_BUTTON_L2:  return GamepadButton::L2;
    case AKEYCODE_BUTTON_R1:  return GamepadButton::R1;
    case AKEYCODE_BUTTON_R2:  return GamepadButton::R2;
    default: return GamepadButton::Unknown;
  }
}

KeyboardKey keyCode(int32_t key)
{
  if (key >= AKEYCODE_A && key <= AKEYCODE_Z) {
    auto i = static_cast<std::underlying_type_t<KeyboardKey>>(KeyboardKey::A) + (key - AKEYCODE_A);
    return static_cast<KeyboardKey>(i);
  }

  switch (key) {
    case AKEYCODE_SPACE: return KeyboardKey::Space;
    // ...
    default: return KeyboardKey::Unknown;
  }
}

class Application
{
  public:
    Application(WindowDelegatePtr windowDelegate, FileSystemPtr fileSystem, Logger& logger);

    void update();
    void onConfigChange();
    void onLeftStickMove(float_t x, float_t y);
    void onRightStickMove(float_t x, float_t y);
    void onKeyDown(KeyboardKey key);
    void onKeyUp(KeyboardKey key);
    void onButtonDown(GamepadButton button);
    void onButtonUp(GamepadButton button);

  private:
    Logger& m_logger;
    WindowDelegatePtr m_windowDelegate;
    FileSystemPtr m_fileSystem;
    render::RendererPtr m_renderer;
    SpatialSystemPtr m_spatialSystem;
    RenderSystemPtr m_renderSystem;
    CollisionSystemPtr m_collisionSystem;
    MapParserPtr m_mapParser;
    ModelLoaderPtr m_modelLoader;
    EntityFactoryPtr m_entityFactory;
    GamePtr m_game;
    Vec2f m_leftStickDelta;
    Vec2f m_rightStickDelta;
};

using ApplicationPtr = std::unique_ptr<Application>;

Application::Application(WindowDelegatePtr windowDelegate, FileSystemPtr fileSystem, Logger& logger)
  : m_logger(logger)
  , m_windowDelegate(std::move(windowDelegate))
  , m_fileSystem(std::move(fileSystem))
{
  m_renderer = createRenderer(*m_fileSystem, *m_windowDelegate, m_logger);
  m_spatialSystem = createSpatialSystem(m_logger);
  m_renderSystem = createRenderSystem(*m_spatialSystem, *m_renderer, m_logger);
  m_collisionSystem = createCollisionSystem(*m_spatialSystem, m_logger);
  m_mapParser = createMapParser(*m_fileSystem, m_logger);
  m_modelLoader = createModelLoader(*m_renderSystem, *m_fileSystem, *m_logger);
  m_entityFactory = createEntityFactory(*m_modelLoader, *m_spatialSystem, *m_renderSystem,
    *m_collisionSystem, *m_fileSystem, *m_logger);

  auto player = createScene(*m_entityFactory, *m_spatialSystem, *m_renderSystem, *m_collisionSystem,
    *m_mapParser, *m_fileSystem, m_logger);

  m_renderSystem->start();
  m_game = createGame(std::move(player), *m_renderSystem, *m_collisionSystem, m_logger);
}

void Application::update()
{
  m_game->onLeftStickMove(m_leftStickDelta);
  m_game->onRightStickMove(m_rightStickDelta);
  m_game->update();
  m_spatialSystem->update();
  m_renderSystem->update();
  m_collisionSystem->update();
}

void Application::onConfigChange()
{
  m_renderer->onResize();
}

void Application::onLeftStickMove(float_t x, float_t y)
{
  m_leftStickDelta = { x, y };
}

void Application::onRightStickMove(float_t x, float_t y)
{
  m_rightStickDelta = { x, y };
}

void Application::onKeyDown(KeyboardKey key)
{
  m_game->onKeyDown(key);
}

void Application::onKeyUp(KeyboardKey key)
{
  m_game->onKeyUp(key);
}

void Application::onButtonDown(GamepadButton button)
{
  m_game->onButtonDown(button);

  if (button == GamepadButton::Y) {
    m_logger.info(STR("Renderer frame rate: " << m_renderer->frameRate()));
  }
}

void Application::onButtonUp(GamepadButton button)
{
  m_game->onButtonUp(button);
}

ApplicationPtr createApplication(android_app& state, Logger& logger)
{
  ASSERT(state.window != nullptr, "Window is NULL");

  auto windowDelegate = createWindowDelegate(*state.window);
  FileSystemPtr fileSystem = createAndroidFileSystem(*state.activity->assetManager);
  return std::make_unique<Application>(std::move(windowDelegate), std::move(fileSystem), logger);
}

class EventHandler
{
  public:
    EventHandler(Logger& logger);

    void setApplication(Application* app);

    void onCommandEvent(int32_t command);
    int32_t onInputEvent(const AInputEvent& event);

    bool isWindowInitialised() const;

  private:
    Logger& m_logger;
    Application* m_app = nullptr;
    bool m_windowInitialised = false;
};

EventHandler::EventHandler(Logger& logger)
  : m_logger(logger)
{
}

void EventHandler::setApplication(Application* app)
{
  m_app = app;
}

void EventHandler::onCommandEvent(int32_t command)
{
  if (command == APP_CMD_INIT_WINDOW) {
    m_windowInitialised = true;
    return;
  }

  if (m_app) {
    switch (command) {
      case APP_CMD_CONFIG_CHANGED:
        m_app->onConfigChange();
        break;
      default: break;
    }
  }
}

int32_t EventHandler::onInputEvent(const AInputEvent& event)
{
  if (m_app) {
    int32_t eventType = AInputEvent_getType(&event);

    if (eventType == AINPUT_EVENT_TYPE_KEY) {
      int32_t action = AKeyEvent_getAction(&event);
      int32_t key = AKeyEvent_getKeyCode(&event);

      if (action == AKEY_EVENT_ACTION_DOWN) {
        auto button = buttonCode(key);
        if (button != GamepadButton::Unknown) {
          m_app->onButtonDown(button);
        }
        else {
          m_app->onKeyDown(keyCode(key));
        }
      }
      else if (action == AKEY_EVENT_ACTION_UP) {
        auto button = buttonCode(key);
        if (button != GamepadButton::Unknown) {
          m_app->onButtonUp(button);
        }
        else {
          m_app->onKeyUp(keyCode(key));
        }
      }

      return 1;
    }
    if (eventType == AINPUT_EVENT_TYPE_MOTION) {
      if (AInputEvent_getSource(&event) == AINPUT_SOURCE_JOYSTICK) {
        float x = AMotionEvent_getAxisValue(&event, AMOTION_EVENT_AXIS_X, 0);
        float y = AMotionEvent_getAxisValue(&event, AMOTION_EVENT_AXIS_Y, 0);

        m_app->onLeftStickMove(x, y);

        float z = AMotionEvent_getAxisValue(&event, AMOTION_EVENT_AXIS_Z, 0);
        float rz = AMotionEvent_getAxisValue(&event, AMOTION_EVENT_AXIS_RZ, 0);

        m_app->onRightStickMove(z, rz);
      }

      return 1;
    }
  }

  return 0;
}

bool EventHandler::isWindowInitialised() const
{
  return m_windowInitialised;
}

bool waitForWindow(android_app& state, EventHandler& handler)
{
  FrameRateLimiter frameRateLimiter{TARGET_FRAME_RATE};

  while (!state.destroyRequested) {
    android_poll_source* source = nullptr;
    ALooper_pollOnce(0, nullptr, nullptr, reinterpret_cast<void**>(&source));

    if (source != nullptr) {
      source->process(&state, source);
    }

    if (handler.isWindowInitialised()) {
      return true;
    }

    frameRateLimiter.wait();
  }

  return false;
}

void handleCommand(android_app* state, int32_t command)
{
  reinterpret_cast<EventHandler*>(state->userData)->onCommandEvent(command);
}

int32_t handleInput(android_app* state, AInputEvent* event)
{
  return reinterpret_cast<EventHandler*>(state->userData)->onInputEvent(*event);
}

} // namespace

void android_main(android_app* state)
{
  auto logger = createAndroidLogger();
  logger->info("Starting Nova");

  EventHandler handler{*logger};

  state->onAppCmd = handleCommand;
  state->onInputEvent = handleInput;
  state->userData = &handler;

  if (!waitForWindow(*state, handler)) {
    return;
  }

  auto application = createApplication(*state, *logger);
  handler.setApplication(application.get());

  FrameRateLimiter frameRateLimiter{TARGET_FRAME_RATE};

  while (!state->destroyRequested) {
    android_poll_source* source = nullptr;
    ALooper_pollOnce(0, nullptr, nullptr, reinterpret_cast<void**>(&source));

    if (source != nullptr) {
      source->process(state, source);
    }

    application->update();

    frameRateLimiter.wait();
  }
}
