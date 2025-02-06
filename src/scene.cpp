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
#include <numeric>
#include <cassert>

namespace
{

double getDoubleValue(const KeyValueMap& map, const std::string& key)
{
  if (map.count(key) == 0) {
    EXCEPTION("Map does not contain '" << key << "' value");
  }

  try {
    return std::stod(map.at(key));
  }
  catch (...) {
    EXCEPTION("Error parsing value with name '" << key << "' as double");
  }
}

std::pair<Vec2f, Vec2f> computeMapBounds(const ObjectData& root)
{
  ASSERT(root.name == "zone", "Expected root object to be a zone");

  Vec2f pathMin{ std::numeric_limits<float_t>::max(), std::numeric_limits<float_t>::max() };
  Vec2f pathMax{ std::numeric_limits<float_t>::lowest(), std::numeric_limits<float_t>::lowest() };

  for (auto& p : root.path.points) {
    pathMin = { std::min(pathMin[0], p[0]), std::min(pathMin[1], p[2]) };
    pathMax = { std::max(pathMax[0], p[0]), std::max(pathMax[1], p[2]) };
  }

  return { pathMin, pathMax };
}

MeshPtr cuboid(float_t w, float_t h, float_t d, const Vec3f& colour)
{
  // TODO: Normals

  MeshPtr mesh = std::make_unique<Mesh>();
  // Viewed from above
  //
  // A +---+ B
  //   |   |
  // D +---+ C
  //
  mesh->vertices = {
    // Bottom face
    {{ 0, 0, 0 }, { 0, 0, 1 }, colour, { 0, 0 }}, // A  0
    {{ w, 0, 0 }, { 0, 0, 1 }, colour, { 1, 0 }}, // B  1
    {{ w, 0, d }, { 0, 0, 1 }, colour, { 1, 1 }}, // C  2
    {{ 0, 0, d }, { 0, 0, 1 }, colour, { 0, 1 }}, // D  3

    // Top face
    {{ 0, h, 0 }, { 0, 0, 1 }, colour, { 0, 0 }}, // A  4
    {{ w, h, 0 }, { 0, 0, 1 }, colour, { 1, 0 }}, // B  5
    {{ w, h, d }, { 0, 0, 1 }, colour, { 1, 1 }}, // C  6
    {{ 0, h, d }, { 0, 0, 1 }, colour, { 0, 1 }}, // D  7
  };
  mesh->indices = {
    0, 1, 2, 0, 2, 3, // Bottom face
    0, 3, 7, 0, 7, 4, // Left face
    2, 1, 5, 2, 5, 6, // Right face
    3, 2, 6, 3, 6, 7, // Near face
    0, 4, 5, 0, 5, 1, // Far face
    7, 6, 5, 7, 5, 4  // Top face
  };

  return mesh;
}

class SceneBuilder
{
  public:
    SceneBuilder(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
      RenderSystem& renderSystem, CollisionSystem& collisionSystem, MapParser& mapParser,
      Logger& logger);

    PlayerPtr createScene();

  private:
    EntityFactory& m_entityFactory;
    SpatialSystem& m_spatialSystem;
    RenderSystem& m_renderSystem;
    CollisionSystem& m_collisionSystem;
    MapParser& m_mapParser;
    Logger& m_logger;
    PlayerPtr m_player = nullptr;

    void constructOriginMarkers();
    void constructInstances(const ObjectData& objectData);
    void constructObject(const ObjectData& obj, const Mat4x4f& parentTransform);
    void constructPlayer(const ObjectData& obj, const Mat4x4f& parentTransform);
    void constructInstance(const ObjectData& obj, const Mat4x4f& parentTransform);
    Mat4x4f constructZone(const ObjectData& obj, const Mat4x4f& parentTransform);
    void constructWall(const ObjectData& obj, const Mat4x4f& parentTransform, bool interior);
};

SceneBuilder::SceneBuilder(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, MapParser& mapParser,
  Logger& logger)
  : m_entityFactory(entityFactory)
  , m_spatialSystem(spatialSystem)
  , m_renderSystem(renderSystem)
  , m_collisionSystem(collisionSystem)
  , m_mapParser(mapParser)
  , m_logger(logger)
{
}

PlayerPtr SceneBuilder::createScene()
{
  auto objectData = m_mapParser.parseMapFile("data/map.svg");

  auto bounds = computeMapBounds(objectData);
  m_logger.info(STR("Map boundary: (" << bounds.first << ") to (" << bounds.second << ")"));

  m_collisionSystem.initialise(bounds.first, bounds.second);

  constructInstances(objectData);
  constructOriginMarkers();

  ASSERT(m_player != nullptr, "Map does not contain player");
  PlayerPtr player = std::move(m_player);
  m_player = nullptr;

  return player;
}

void SceneBuilder::constructOriginMarkers()
{
  auto construct = [this](float_t x, float_t z, const Vec3f& colour) {
    EntityId id = System::nextId();

    MeshPtr mesh = cuboid(0.5f, 5.f, 0.5f, colour);
    MeshId meshId = m_renderSystem.addMesh(std::move(mesh));
    CRenderPtr render = std::make_unique<CRender>(id);
    render->mesh = meshId;
    m_renderSystem.addComponent(std::move(render));

    CSpatialPtr spatial = std::make_unique<CSpatial>(id);
    spatial->setTransform(translationMatrix4x4(Vec3f{ x, 0, z }));
    m_spatialSystem.addComponent(std::move(spatial));
  };

  construct(0, 0, { 1, 0, 0 });
  construct(10, 0, { 0, 1, 0 });
  construct(0, 10, { 0, 0, 1 });
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
    objTransform = constructZone(obj, parentTransform);
  }
  else if (obj.name == "wall") {
    constructWall(obj, parentTransform, false);
    return;
  }
  else if (obj.name == "interior") {
    constructWall(obj, parentTransform, true);
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
  m_entityFactory.constructEntity(obj.name, m);
}

void SceneBuilder::constructPlayer(const ObjectData& obj, const Mat4x4f& parentTransform)
{
  auto m = parentTransform * obj.transform * transformFromTriangle(obj.path);

  // Expecting a matrix of the form
  //    cos(a),   0,        sin(a),   tx,
  //    0,        1,        0,        0,
  //    -sin(a),  0,        cos(a),   tz
  //    0,        0,        0,        1
  float_t yaw = 2.f * PI - atan2(m.at(2, 0), m.at(0, 0));

  float_t x = m.at(0, 3);
  float_t y = m.at(1, 3);
  float_t z = m.at(2, 3);

  m_player = createPlayer(m_renderSystem.camera());
  m_player->rotate(0, yaw);
  m_player->setPosition({ x, y, z });
}

MeshPtr mergeMeshes(const Mesh& A, const Mesh& B)
{
  MeshPtr mesh = std::make_unique<Mesh>();

  mesh->vertices.insert(mesh->vertices.end(), A.vertices.begin(), A.vertices.end());
  mesh->vertices.insert(mesh->vertices.end(), B.vertices.begin(), B.vertices.end());
  mesh->indices.insert(mesh->indices.end(), A.indices.begin(), A.indices.end());
  size_t n = A.vertices.size();
  std::transform(B.indices.begin(), B.indices.end(), std::back_inserter(mesh->indices),
    [n](uint16_t i) { return i + n; });

  return mesh;
}

MeshPtr createBottomFace(const std::vector<Vec4f>& points, const Vec3f& colour)
{
  MeshPtr mesh = std::make_unique<Mesh>();

  std::vector<Vec4f> vertices;
  for (auto i = points.rbegin(); i != points.rend(); ++i) {
    auto& p = *i;
    vertices.push_back(p);
    // TODO: Normals and UVs
    Vertex vertex{Vec3f{ p[0], p[1], p[2] }, Vec3f{ 0, 0, 0 }, colour, Vec2f{ 0, 0 }};
    mesh->vertices.push_back(vertex);
  }

  mesh->indices = triangulatePoly(vertices);

  return mesh;
}

MeshPtr createTopFace(const std::vector<Vec4f>& points, const Vec3f& colour, float_t height)
{
  auto mesh = createBottomFace(points, colour);

  for (auto& p : mesh->vertices) {
    p.pos[1] += height;
  }

  std::reverse(mesh->indices.begin(), mesh->indices.end());

  return mesh;
}

void createSideFaces(Mesh& mesh)
{
  assert(mesh.vertices.size() % 2 == 0);

  uint16_t n = static_cast<uint16_t>(mesh.vertices.size() / 2);
  for (uint16_t i = 0; i < n; ++i) {
    uint16_t j = n + i;
    uint16_t nextI = (i + 1) % n;
    uint16_t nextJ = n + nextI;

    mesh.indices.insert(mesh.indices.end(), { i, j, nextJ, i, nextJ, nextI });
  }
}

Mat4x4f SceneBuilder::constructZone(const ObjectData& obj, const Mat4x4f& parentTransform)
{
  EntityId id = System::nextId();
  auto floorHeight = metresToWorldUnits(getDoubleValue(obj.values, "floor"));
  float_t height = floorHeight;
  auto offset = identityMatrix<float_t, 4>();
  if (floorHeight == 0) {
    height = 1.f;
    offset = translationMatrix4x4(Vec3f{ 0, -height, 0 });
  }

  CSpatialPtr spatial = std::make_unique<CSpatial>(id);
  spatial->setTransform( parentTransform * offset * obj.transform);
  m_spatialSystem.addComponent(std::move(spatial));

  Vec3f colour{ 0.15, 0.1, 0.08 };

  auto bottomFace = createBottomFace(obj.path.points, colour);
  auto topFace = createTopFace(obj.path.points, colour, height);

  auto mesh = mergeMeshes(*bottomFace, *topFace);
  createSideFaces(*mesh);

  CRenderPtr render = std::make_unique<CRender>(id);
  render->mesh = m_renderSystem.addMesh(std::move(mesh));
  m_renderSystem.addComponent(std::move(render));

  return translationMatrix4x4(Vec3f{ 0, floorHeight, 0 }) * obj.transform;
}

void SceneBuilder::constructWall(const ObjectData& obj, const Mat4x4f& parentTransform,
  bool interior)
{
  // TODO
}

} // namespace

PlayerPtr createScene(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, MapParser& mapParser,
  Logger& logger)
{
  SceneBuilder builder(entityFactory, spatialSystem, renderSystem, collisionSystem, mapParser,
    logger);

  return builder.createScene();
}
