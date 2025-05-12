#include "scene.hpp"
#include "entity_factory.hpp"
#include "spatial_system.hpp"
#include "render_system.hpp"
#include "collision_system.hpp"
#include "map_parser.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "units.hpp"
#include "player.hpp"
#include "file_system.hpp"
#include "xml.hpp"
#include "terrain.hpp"
#include <numeric>
#include <array>
#include <cassert>

using render::BufferUsage;
namespace MeshFeatures = render::MeshFeatures;
using render::MeshFeatureSet;
namespace MaterialFeatures = render::MaterialFeatures;
using render::MaterialFeatureSet;
using render::TexturePtr;
using render::Material;
using render::MaterialPtr;
using render::MeshPtr;

namespace
{

class SceneBuilder
{
  public:
    SceneBuilder(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
      RenderSystem& renderSystem, CollisionSystem& collisionSystem, MapParser& mapParser,
      const FileSystem& fileSystem, Logger& logger);

    PlayerPtr createScene();

  private:
    EntityFactory& m_entityFactory;
    SpatialSystem& m_spatialSystem;
    RenderSystem& m_renderSystem;
    CollisionSystem& m_collisionSystem;
    MapParser& m_mapParser;
    const FileSystem& m_fileSystem;
    Logger& m_logger;
    PlayerPtr m_player = nullptr;
    TerrainPtr m_terrain = nullptr;

    void constructOriginMarkers();
    void constructInstances(const ObjectData& objectData);
    void constructObject(const ObjectData& obj, const Mat4x4f& parentTransform);
    void constructPlayer(const ObjectData& obj, const Mat4x4f& parentTransform);
    void constructInstance(const ObjectData& obj, const Mat4x4f& parentTransform);
    void constructSky();
};

SceneBuilder::SceneBuilder(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, MapParser& mapParser,
  const FileSystem& fileSystem, Logger& logger)
  : m_entityFactory(entityFactory)
  , m_spatialSystem(spatialSystem)
  , m_renderSystem(renderSystem)
  , m_collisionSystem(collisionSystem)
  , m_mapParser(mapParser)
  , m_fileSystem(fileSystem)
  , m_logger(logger)
{
  m_terrain = createTerrain(m_entityFactory, m_spatialSystem, m_renderSystem, m_collisionSystem,
    m_fileSystem, m_logger);
}

PlayerPtr SceneBuilder::createScene()
{
  auto scene = parseXml(m_fileSystem.readFile("scenes/scene1.xml"));

  m_entityFactory.loadMaterials(*scene->child("materials"));
  m_entityFactory.loadModels(*scene->child("models"));
  m_entityFactory.loadEntityDefinitions(*scene->child("entities"));

  auto objectData = m_mapParser.parseMapFile(scene->attribute("map"));

  auto bounds = computeBounds(objectData);
  bounds.first -= Vec2f{ 1.f, 1.f };
  bounds.second += Vec2f{ 1.f, 1.f };

  m_logger.info(STR("Map boundary: (" << bounds.first << ") to (" << bounds.second << ")"));

  m_collisionSystem.initialise(bounds.first, bounds.second);

  constructInstances(objectData);
  constructSky();
  constructOriginMarkers();

  ASSERT(m_player != nullptr, "Map does not contain player");
  PlayerPtr player = std::move(m_player);
  m_player = nullptr;

  return player;
}

void SceneBuilder::constructSky()
{
  EntityId entityId = System::nextId();

  auto render = std::make_unique<CRenderSkybox>(entityId);
  auto mesh = render::cuboid(9999, 9999, 9999, Vec2f{ 1, 1 });
  mesh->attributeBuffers.resize(1); // Just keep the positions
  mesh->featureSet.vertexLayout = { BufferUsage::AttrPosition };
  mesh->featureSet.flags.set(MeshFeatures::IsSkybox, true);
  uint16_t* indexData = reinterpret_cast<uint16_t*>(mesh->indexBuffer.data.data());
  std::reverse(indexData, indexData + mesh->indexBuffer.data.size() / sizeof(uint16_t));
  std::array<TexturePtr, 6> textures{
    render::loadTexture(m_fileSystem.readFile("resources/textures/skybox/right.png")),
    render::loadTexture(m_fileSystem.readFile("resources/textures/skybox/left.png")),
    render::loadTexture(m_fileSystem.readFile("resources/textures/skybox/top.png")),
    render::loadTexture(m_fileSystem.readFile("resources/textures/skybox/bottom.png")),
    render::loadTexture(m_fileSystem.readFile("resources/textures/skybox/front.png")),
    render::loadTexture(m_fileSystem.readFile("resources/textures/skybox/back.png"))
  };
  auto material = std::make_unique<Material>(MaterialFeatureSet{});
  material->featureSet.flags.set(MaterialFeatures::HasCubeMap);
  material->cubeMap.id = m_renderSystem.addCubeMap(std::move(textures));
  m_renderSystem.compileShader(mesh->featureSet, material->featureSet);
  render->model = Submodel{
    .mesh = m_renderSystem.addMesh(std::move(mesh)),
    .material = m_renderSystem.addMaterial(std::move(material)),
    .skin = {}
  };
  m_renderSystem.addComponent(std::move(render));

  CSpatialPtr spatial = std::make_unique<CSpatial>(entityId, identityMatrix<float_t, 4>(), 10000.f);
  m_spatialSystem.addComponent(std::move(spatial));
}

void SceneBuilder::constructOriginMarkers()
{
  auto construct = [this](float_t x, float_t z, const Vec4f& colour) {
    EntityId id = System::nextId();

    float_t w = metresToWorldUnits(1);
    float_t d = metresToWorldUnits(1);
    float_t h = metresToWorldUnits(20);

    MaterialPtr material = std::make_unique<Material>(MaterialFeatureSet{});
    material->colour = colour;

    MeshPtr mesh = render::cuboid(w, h, d, Vec2f{ 1, 1 });
    mesh->attributeBuffers.resize(2); // Keep just positions and normals
    mesh->featureSet.vertexLayout = { BufferUsage::AttrPosition, BufferUsage::AttrNormal };
    m_renderSystem.compileShader(mesh->featureSet, material->featureSet);
    auto meshId = m_renderSystem.addMesh(std::move(mesh));
    CRenderModelPtr render = std::make_unique<CRenderModel>(id);
    render->submodels = {
      Submodel{
        .mesh = meshId,
        .material = m_renderSystem.addMaterial(std::move(material)),
        .skin = {}
      }
    };
    m_renderSystem.addComponent(std::move(render));

    CSpatialPtr spatial = std::make_unique<CSpatial>(id, translationMatrix4x4(Vec3f{ x, 0, z }),
      metresToWorldUnits(0.5f));
    m_spatialSystem.addComponent(std::move(spatial));
  };

  float_t distanceFromOrigin = metresToWorldUnits(5);
  construct(0, 0, { 1, 0, 0, 1 });
  construct(distanceFromOrigin, 0, { 0, 1, 0, 1 });
  construct(0, distanceFromOrigin, { 0, 0, 1, 1 });
}

void SceneBuilder::constructInstances(const ObjectData& objectData)
{
  constructObject(objectData, identityMatrix<float_t, 4>());
}

void SceneBuilder::constructObject(const ObjectData& obj, const Mat4x4f& parentTransform)
{
  m_logger.debug(STR("Constructing " << obj.name << " object"));

  auto objTransform = obj.transform;

  if (obj.name == "player") {
    constructPlayer(obj, parentTransform);
    return;
  }
  else if (obj.name == "zone") {
    objTransform = m_terrain->constructZone(obj, parentTransform);
  }
  else if (obj.name == "wall") {
    m_terrain->constructWall(obj, parentTransform, false);
    return;
  }
  else if (obj.name == "interior") {
    m_terrain->constructWall(obj, parentTransform, true);
    return;
  }
  else {
    constructInstance(obj, parentTransform);
  }

  for (auto& child : obj.children) {
    constructObject(child, parentTransform * objTransform);
  }
}

void SceneBuilder::constructInstance(const ObjectData& obj, const Mat4x4f& parentTransform)
{
  Mat4x4f m = parentTransform * obj.transform * transformFromTriangle(obj.path);
  m_entityFactory.constructEntity(obj, m);
}

void SceneBuilder::constructPlayer(const ObjectData& obj, const Mat4x4f& parentTransform)
{
  auto m = parentTransform * obj.transform * transformFromTriangle(obj.path);

  // Expecting a matrix of the form
  //    cos(a),   0,        sin(a),   tx,
  //    0,        1,        0,        0,
  //    -sin(a),  0,        cos(a),   tz
  //    0,        0,        0,        1
  float_t yaw = 2.f * PIf - atan2(m.at(2, 0), m.at(0, 0));

  float_t x = m.at(0, 3);
  float_t y = m.at(1, 3);
  float_t z = m.at(2, 3);

  m_player = createPlayer(m_renderSystem.camera());
  m_player->rotate(0, yaw);
  m_player->setPosition({ x, y, z });
}

} // namespace

PlayerPtr createScene(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, MapParser& mapParser,
  const FileSystem& fileSystem, Logger& logger)
{
  SceneBuilder builder(entityFactory, spatialSystem, renderSystem, collisionSystem, mapParser,
    fileSystem, logger);

  return builder.createScene();
}
