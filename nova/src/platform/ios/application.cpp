#include "platform/ios/application.h"
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
#include "window_delegate.hpp"
#include "platform_paths.hpp"
#include <iostream>

namespace
{

class ApplicationImpl : public Application
{
  public:
    ApplicationImpl(const char* bundlePath, WindowDelegatePtr windowDelegate);

    void update() override;

  private:
    PlatformPathsPtr m_platformPaths;
    WindowDelegatePtr m_windowDelegate;
    LoggerPtr m_logger;
    RendererPtr m_renderer;
    SpatialSystemPtr m_spatialSystem;
    RenderSystemPtr m_renderSystem;
    CollisionSystemPtr m_collisionSystem;
    MapParserPtr m_mapParser;
    EntityFactoryPtr m_entityFactory;
    GamePtr m_game;
};

ApplicationImpl::ApplicationImpl(const char* bundlePath, WindowDelegatePtr windowDelegate)
  : m_windowDelegate(std::move(windowDelegate))
{
  std::filesystem::path root{bundlePath};

  DirectoryMap directories;
  directories["data"] = root / "data";
  directories["shaders"] = root / "data/shaders";

  m_platformPaths = std::make_unique<PlatformPaths>(directories);

  m_logger = createLogger(std::cerr, std::cerr, std::cout, std::cout);
  m_renderer = createRenderer(*m_platformPaths, *m_windowDelegate, *m_logger);
  m_spatialSystem = createSpatialSystem(*m_logger);
  m_renderSystem = createRenderSystem(*m_spatialSystem, *m_renderer, *m_logger);
  m_collisionSystem = createCollisionSystem(*m_spatialSystem, *m_logger);
  m_mapParser = createMapParser(*m_logger);
  m_entityFactory = createEntityFactory(*m_spatialSystem, *m_renderSystem, *m_collisionSystem,
    *m_platformPaths, *m_logger);

  auto player = createScene(*m_entityFactory, *m_spatialSystem, *m_renderSystem, *m_collisionSystem,
    *m_mapParser, *m_platformPaths, *m_logger);

  m_renderSystem->start();
  m_game = createGame(std::move(player), *m_collisionSystem, *m_logger);
}

void ApplicationImpl::update()
{
  m_game->update();
  m_spatialSystem->update();
  m_renderSystem->update();
  m_collisionSystem->update();
}

}

ApplicationPtr createApplication(const char* bundlePath, WindowDelegatePtr windowDelegate)
{
  return std::make_unique<ApplicationImpl>(bundlePath, std::move(windowDelegate));
}
