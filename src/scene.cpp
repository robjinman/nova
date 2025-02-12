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

float_t getFloatValue(const KeyValueMap& map, const std::string& key)
{
  if (map.count(key) == 0) {
    EXCEPTION("Map does not contain '" << key << "' value");
  }

  try {
    return parseFloat<float_t>(map.at(key));
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
    RenderItemId m_groundMaterial;
    RenderItemId m_wallMaterial;

    void createTerrainMaterials();
    void constructOriginMarkers();
    void constructInstances(const ObjectData& objectData);
    void constructObject(const ObjectData& obj, const Mat4x4f& parentTransform);
    void constructPlayer(const ObjectData& obj, const Mat4x4f& parentTransform);
    void constructInstance(const ObjectData& obj, const Mat4x4f& parentTransform);
    Mat4x4f constructZone(const ObjectData& obj, const Mat4x4f& parentTransform);
    void constructWall(const ObjectData& obj, const Mat4x4f& parentTransform, bool interior);
    void constructSky();
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

  createTerrainMaterials();
  constructInstances(objectData);
  constructSky();
  constructOriginMarkers();

  ASSERT(m_player != nullptr, "Map does not contain player");
  PlayerPtr player = std::move(m_player);
  m_player = nullptr;

  return player;
}

void SceneBuilder::createTerrainMaterials()
{
  auto groundTexture = loadTexture("./data/resources/textures/ground.png");
  auto groundMaterial = std::make_unique<Material>();
  groundMaterial->texture = m_renderSystem.addTexture(std::move(groundTexture));
  m_groundMaterial = m_renderSystem.addMaterial(std::move(groundMaterial));

  auto wallTexture = loadTexture("./data/resources/textures/bricks.png");
  auto wallMaterial = std::make_unique<Material>();
  wallMaterial->texture = m_renderSystem.addTexture(std::move(wallTexture));
  m_wallMaterial = m_renderSystem.addMaterial(std::move(wallMaterial));
}

void SceneBuilder::constructSky()
{
  EntityId entityId = System::nextId();

  auto render = std::make_unique<CRender>(entityId, CRenderType::skybox);
  auto mesh = cuboid(10000, 10000, 10000, { 1, 1, 1 }, Vec2f{ 1, 1 });
  std::reverse(mesh->indices.begin(), mesh->indices.end());
  std::array<TexturePtr, 6> textures{
    loadTexture("./data/resources/textures/skybox/right.png"),
    loadTexture("./data/resources/textures/skybox/left.png"),
    loadTexture("./data/resources/textures/skybox/top.png"),
    loadTexture("./data/resources/textures/skybox/bottom.png"),
    loadTexture("./data/resources/textures/skybox/front.png"),
    loadTexture("./data/resources/textures/skybox/back.png")
  };
  auto material = std::make_unique<Material>();
  material->cubeMap = m_renderSystem.addCubeMap(std::move(textures));
  render->mesh = m_renderSystem.addMesh(std::move(mesh));
  render->material = m_renderSystem.addMaterial(std::move(material));
  m_renderSystem.addComponent(std::move(render));

  CSpatialPtr spatial = std::make_unique<CSpatial>(entityId);
  m_spatialSystem.addComponent(std::move(spatial));
}

void SceneBuilder::constructOriginMarkers()
{
  auto construct = [this](float_t x, float_t z, const Vec3f& colour) {
    EntityId id = System::nextId();

    float_t w = metresToWorldUnits(1);
    float_t d = metresToWorldUnits(1);
    float_t h = metresToWorldUnits(20);

    MeshPtr mesh = cuboid(w, h, d, colour, Vec2f{ 1, 1 });
    auto meshId = m_renderSystem.addMesh(std::move(mesh));
    CRenderPtr render = std::make_unique<CRender>(id, CRenderType::regular);
    render->mesh = meshId;
    m_renderSystem.addComponent(std::move(render));

    CSpatialPtr spatial = std::make_unique<CSpatial>(id);
    spatial->setTransform(translationMatrix4x4(Vec3f{ x, 0, z }));
    m_spatialSystem.addComponent(std::move(spatial));
  };

  float_t distanceFromOrigin = metresToWorldUnits(5);
  construct(0, 0, { 1, 0, 0 });
  construct(distanceFromOrigin, 0, { 0, 1, 0 });
  construct(0, distanceFromOrigin, { 0, 0, 1 });
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

  Vec2f textureSize = metresToWorldUnits(Vec2f{ 4, 4 });

  std::vector<Vec4f> vertices;
  for (auto i = points.rbegin(); i != points.rend(); ++i) {
    auto& p = *i;
    vertices.push_back(p);
    Vec2f uv{ p[0] / textureSize[0], p[2] / textureSize[1] };
    Vertex vertex{ p.sub<3>(), Vec3f{ 0, -1, 0 }, colour, uv };
    mesh->vertices.push_back(vertex);
  }

  mesh->indices = triangulatePoly(vertices);

  return mesh;
}

MeshPtr createTopFace(const std::vector<Vec4f>& points, const Vec3f& colour, float_t height)
{
  auto mesh = createBottomFace(points, colour);
  Vec3f normal{ 0, 1, 0 };

  for (auto& p : mesh->vertices) {
    p.pos[1] += height;
    p.normal = normal;
  }

  std::reverse(mesh->indices.begin(), mesh->indices.end());

  return mesh;
}

void createSideFaces(Mesh& mesh)
{
  assert(mesh.vertices.size() % 2 == 0);

  Vec2f textureSize = metresToWorldUnits(Vec2f{ 4, 4 });

  uint16_t n = static_cast<uint16_t>(mesh.vertices.size() / 2);
  float_t distance = 0.f;
  for (uint16_t i = 0; i < n; ++i) {
    uint16_t j = n + i;
    uint16_t nextI = (i + 1) % n;
    uint16_t nextJ = n + nextI;

    // Face
    Vertex A = mesh.vertices[i];
    Vertex B = mesh.vertices[nextI];
    Vertex C = mesh.vertices[j];
    Vertex D = mesh.vertices[nextJ];

    Vec3f normal = -(A.pos - B.pos).cross(A.pos - C.pos).normalise();
    A.normal = normal;
    B.normal = normal;
    C.normal = normal;
    D.normal = normal;

    float_t edgeLength = (B.pos - A.pos).magnitude();
    float_t height = C.pos[1] - A.pos[1];

    A.texCoord = Vec2f{ distance / textureSize[0], height / textureSize[1] };
    B.texCoord = Vec2f{ (distance + edgeLength) / textureSize[0], height / textureSize[1] };
    C.texCoord = Vec2f{ distance / textureSize[0], 0 };
    D.texCoord = Vec2f{ (distance + edgeLength) / textureSize[0], 0 };

    distance += edgeLength;

    uint16_t idx = mesh.vertices.size();
    mesh.vertices.insert(mesh.vertices.end(), { A, B, C, D });

    mesh.indices.push_back(idx);      // A
    mesh.indices.push_back(idx + 2);  // C
    mesh.indices.push_back(idx + 1);  // B

    mesh.indices.push_back(idx + 1);  // B
    mesh.indices.push_back(idx + 2);  // C
    mesh.indices.push_back(idx + 3);  // D
  }
}

Mat4x4f SceneBuilder::constructZone(const ObjectData& obj, const Mat4x4f& parentTransform)
{
  EntityId entityId = System::nextId();
  float_t floorHeight = metresToWorldUnits(getFloatValue(obj.values, "floor"));
  float_t height = floorHeight;
  auto offset = identityMatrix<float_t, 4>();
  if (floorHeight == 0) {
    height = 1.f;
    offset = translationMatrix4x4(Vec3f{ 0, -height, 0 });
  }

  CSpatialPtr spatial = std::make_unique<CSpatial>(entityId);
  spatial->setTransform( parentTransform * offset * obj.transform);
  m_spatialSystem.addComponent(std::move(spatial));

  Vec3f colour{ 0.15, 0.1, 0.08 };

  auto bottomFace = createBottomFace(obj.path.points, colour);
  auto topFace = createTopFace(obj.path.points, colour, height);

  auto mesh = mergeMeshes(*bottomFace, *topFace);
  createSideFaces(*mesh);

  CRenderPtr render = std::make_unique<CRender>(entityId, CRenderType::regular);
  render->mesh = m_renderSystem.addMesh(std::move(mesh));
  render->material = m_groundMaterial;
  m_renderSystem.addComponent(std::move(render));

  CCollisionPtr collision = std::make_unique<CCollision>(entityId);
  collision->height = height;
  std::transform(obj.path.points.begin(), obj.path.points.end(),
    std::back_inserter(collision->perimeter), [](const Vec4f& p) { return Vec2f{ p[0], p[2] }; });
  m_collisionSystem.addComponent(std::move(collision));

  return translationMatrix4x4(Vec3f{ 0, floorHeight, 0 }) * obj.transform;
}

void SceneBuilder::constructWall(const ObjectData& obj, const Mat4x4f& parentTransform,
  bool /*interior*/) // TODO: Interiors
{
  const float_t wallThickness = metresToWorldUnits(0.4);
  Vec2f textureSize = metresToWorldUnits(Vec2f{ 4, 4 });

  float_t wallHeight = metresToWorldUnits(getFloatValue(obj.values, "height"));
  const Vec3f colour{ 0.3f, 0.25f, 0.2f };

  auto& points = obj.path.points;

  if (points.size() < 2) {
    EXCEPTION("Wall path must have at least 2 points");
  }

  size_t n = obj.path.closed ? points.size() + 1 : points.size();

  for (size_t i = 1; i < n; ++i) {
    Vec4f p1 = points[i - 1];
    Vec4f p2 = points[i % points.size()];
    Vec3f vec = p2.sub<3>() - p1.sub<3>();
    float_t distance = vec.magnitude();
    Vec3f v = vec.normalise();

    Mat4x4f m{
      v[2],   0,  v[0],   p1[0],
      0,      1,  0,      0,
      -v[0],  0,  v[2],   p1[2],
      0,      0,  0,      1
    };

    float_t w = wallThickness;
    float_t h = wallHeight;
    float_t d = distance;

    Mat4x4f shift = translationMatrix4x4(Vec3f{ w / 2.f, h / 2.f, d / 2.f });

    EntityId entityId = System::nextId();

    CSpatialPtr spatial = std::make_unique<CSpatial>(entityId);
    spatial->setTransform(parentTransform * obj.transform * m * shift);
    m_spatialSystem.addComponent(std::move(spatial));

    CRenderPtr render = std::make_unique<CRender>(entityId, CRenderType::regular);
    auto mesh = cuboid(wallThickness, wallHeight, distance, colour, textureSize);
    render->mesh = m_renderSystem.addMesh(std::move(mesh));
    render->material = m_wallMaterial;
    m_renderSystem.addComponent(std::move(render));

    CCollisionPtr collision = std::make_unique<CCollision>(entityId);
    collision->height = wallHeight;
    collision->perimeter = {
      { -w / 2.f, -d / 2.f },
      { w / 2.f, -d / 2.f },
      { w / 2.f, d / 2.f },
      { -w / 2.f, d / 2.f }
    };
    m_collisionSystem.addComponent(std::move(collision));
  }
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
