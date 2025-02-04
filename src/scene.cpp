#include "scene.hpp"
#include "spatial_system.hpp"
#include "render_system.hpp"
#include "collision_system.hpp"
#include "map_parser.hpp"
#include "logger.hpp"
#include "utils.hpp"

namespace
{

std::pair<Vec2f, Vec2f> computeMapBounds(const ObjectData& root)
{
  ASSERT(root.name == "zone", "Expected root object to be a zone");

  Vec2f pathMin{ std::numeric_limits<float_t>::max(), std::numeric_limits<float_t>::max() };
  Vec2f pathMax{ std::numeric_limits<float_t>::lowest(), std::numeric_limits<float_t>::lowest() };

  for (auto& p : root.path.points) {
    pathMin = { std::min(pathMin[0], p[0]), std::min(pathMin[1], p[1]) };
    pathMax = { std::max(pathMax[0], p[0]), std::max(pathMax[1], p[1]) };
  }

  return { pathMin, pathMax };
}

class SceneBuilder
{
  public:
    SceneBuilder(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
      CollisionSystem& collisionSystem, MapParser& mapParser, Logger& logger);

    void createScene();

  private:
    SpatialSystem& m_spatialSystem;
    RenderSystem& m_renderSystem;
    CollisionSystem& m_collisionSystem;
    MapParser& m_mapParser;
    Logger& m_logger;
};

SceneBuilder::SceneBuilder(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, MapParser& mapParser, Logger& logger)
  : m_spatialSystem(spatialSystem)
  , m_renderSystem(renderSystem)
  , m_collisionSystem(collisionSystem)
  , m_mapParser(mapParser)
  , m_logger(logger)
{
}

void SceneBuilder::createScene()
{
  auto objectData = m_mapParser.parseMapFile("data/map.svg");

  auto bounds = computeMapBounds(objectData);
  m_logger.info(STR("Map boundary: (" << bounds.first << ") to (" << bounds.second << ")"));

  m_collisionSystem.initialise(bounds.first, bounds.second);

  // TODO
}

} // namespace

void createScene(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, MapParser& mapParser, Logger& logger)
{
  SceneBuilder builder(spatialSystem, renderSystem, collisionSystem, mapParser, logger);
  builder.createScene();
}
