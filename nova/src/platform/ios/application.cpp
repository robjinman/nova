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
#include "model_loader.hpp"
#include "time.hpp"
#include "window_delegate.hpp"
#include "file_system.hpp"
#include <iostream>

FileSystemPtr createDefaultFileSystem(const std::filesystem::path& dataRootDir);

namespace
{

class ApplicationImpl : public Application
{
  public:
    ApplicationImpl(const char* bundlePath, WindowDelegatePtr windowDelegate);

    void onViewResize() override;
    void update() override;

  private:
    FileSystemPtr m_fileSystem;
    WindowDelegatePtr m_windowDelegate;
    LoggerPtr m_logger;
    render::RendererPtr m_renderer;
    SpatialSystemPtr m_spatialSystem;
    RenderSystemPtr m_renderSystem;
    CollisionSystemPtr m_collisionSystem;
    MapParserPtr m_mapParser;
    ModelLoaderPtr m_modelLoader;
    EntityFactoryPtr m_entityFactory;
    GamePtr m_game;
};

ApplicationImpl::ApplicationImpl(const char* bundlePath, WindowDelegatePtr windowDelegate)
  : m_windowDelegate(std::move(windowDelegate))
{
  m_fileSystem = createDefaultFileSystem(std::filesystem::path{bundlePath} / "data");
  m_logger = createLogger(std::cerr, std::cerr, std::cout, std::cout);
  m_renderer = createRenderer(*m_fileSystem, *m_windowDelegate, *m_logger);
  m_spatialSystem = createSpatialSystem(*m_logger);
  m_renderSystem = createRenderSystem(*m_spatialSystem, *m_renderer, *m_logger);
  m_collisionSystem = createCollisionSystem(*m_spatialSystem, *m_logger);
  m_mapParser = createMapParser(*m_fileSystem, *m_logger);
  m_modelLoader = createModelLoader(*m_renderSystem, *m_fileSystem, *m_logger);
  m_entityFactory = createEntityFactory(*m_modelLoader, *m_spatialSystem, *m_renderSystem,
    *m_collisionSystem, *m_fileSystem, *m_logger);

  auto player = createScene(*m_entityFactory, *m_spatialSystem, *m_renderSystem, *m_collisionSystem,
    *m_mapParser, *m_fileSystem, *m_logger);

  m_renderSystem->start();
  m_game = createGame(std::move(player), *m_renderSystem, *m_collisionSystem, *m_logger);
}

void ApplicationImpl::onViewResize()
{
  m_renderer->onResize();
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
