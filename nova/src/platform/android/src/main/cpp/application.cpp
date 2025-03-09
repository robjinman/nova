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
#include "units.hpp"
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <iostream>

const char* LOG_TAG = "eggplant";

WindowDelegatePtr createWindowDelegate(ANativeWindow& window);
FileSystemPtr createAndroidFileSystem(AAssetManager& assetManager);

namespace
{

class Application
{
  public:
    Application(WindowDelegatePtr windowDelegate, FileSystemPtr fileSystem);

    void update();

  private:
    WindowDelegatePtr m_windowDelegate;
    FileSystemPtr m_fileSystem;
    LoggerPtr m_logger;
    RendererPtr m_renderer;
    SpatialSystemPtr m_spatialSystem;
    RenderSystemPtr m_renderSystem;
    CollisionSystemPtr m_collisionSystem;
    MapParserPtr m_mapParser;
    EntityFactoryPtr m_entityFactory;
    GamePtr m_game;
};

using ApplicationPtr = std::unique_ptr<Application>;

Application::Application(WindowDelegatePtr windowDelegate, FileSystemPtr fileSystem)
  : m_windowDelegate(std::move(windowDelegate))
  , m_fileSystem(std::move(fileSystem))
{
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
}

void Application::update()
{
  m_game->update();
  m_spatialSystem->update();
  m_renderSystem->update();
  m_collisionSystem->update();
}

ApplicationPtr createApplication(android_app& state)
{
  ASSERT(state.window != nullptr, "Window is NULL");

  auto windowDelegate = createWindowDelegate(*state.window);
  FileSystemPtr fileSystem = createAndroidFileSystem(*state.activity->assetManager);
  return std::make_unique<Application>(std::move(windowDelegate), std::move(fileSystem));
}

bool waitForWindow(android_app& state)
{
  FrameRateLimiter frameRateLimiter{TARGET_FRAME_RATE};

  while (!state.destroyRequested) {
    android_poll_source* source = nullptr;
    ALooper_pollOnce(0, nullptr, nullptr, reinterpret_cast<void**>(&source));

    if (source != nullptr) {
      source->process(&state, source);
    }

    if (state.userData) {
      return true;
    }

    frameRateLimiter.wait();
  }

  return false;
}

void handleCommand(android_app* state, int32_t command)
{
  switch (command) {
    case APP_CMD_INIT_WINDOW:
      state->userData = reinterpret_cast<void*>(0xdeadbeef);
      break;
    default: break;
  }
}

} // namespace

void android_main(android_app* state)
{
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Starting Nova");

  state->onAppCmd = handleCommand;
  state->userData = nullptr;

  if (!waitForWindow(*state)) {
    return;
  }

  auto application = createApplication(*state);

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
