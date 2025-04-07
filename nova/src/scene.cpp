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
#include <numeric>
#include <array>
#include <cassert>

namespace
{

float_t computeRadius(const std::span<const Vec3f>& positions)
{
  float_t furthestX = std::numeric_limits<float_t>::lowest();
  float_t furthestZ = std::numeric_limits<float_t>::lowest();
  for (const auto& pos : positions) {
    if (fabs(pos[0]) > furthestX) {
      furthestX = fabs(pos[0]);
    }
    if (fabs(pos[2]) > furthestZ) {
      furthestZ = fabs(pos[2]);
    }
  }
  return sqrt(furthestX * furthestX + furthestZ * furthestZ);
}

void centreObject(ObjectData& object)
{
  Vec2f centre{};
  for (auto& p : object.path.points) {
    centre += { p[0], p[2] };
  }
  centre = centre / static_cast<float_t>(object.path.points.size());
  for (auto& p : object.path.points) {
    p[0] -= centre[0];
    p[2] -= centre[1];
  }
  object.transform = object.transform * translationMatrix4x4(Vec3f{ centre[0], 0.f, centre[1] });
}

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

std::pair<Vec2f, Vec2f> computeBounds(const ObjectData& root)
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
    void fillArea(const ObjectData& area, const Mat4x4f& transform, float_t height,
      const std::string& entityType);
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
}

PlayerPtr SceneBuilder::createScene()
{
  auto scene = parseXml(m_fileSystem.readFile("scenes/scene1.xml"));

  m_entityFactory.loadModels(*scene->child("models"));
  m_entityFactory.loadEntityDefinitions(*scene->child("entities"));

  auto objectData = m_mapParser.parseMapFile(scene->attribute("map"));

  auto bounds = computeBounds(objectData);
  bounds.first -= Vec2f{ 1.f, 1.f };
  bounds.second += Vec2f{ 1.f, 1.f };

  m_logger.info(STR("Map boundary: (" << bounds.first << ") to (" << bounds.second << ")"));

  m_collisionSystem.initialise(bounds.first, bounds.second);

  createTerrainMaterials();
  constructInstances(objectData);
  constructSky();
  //constructOriginMarkers();

  ASSERT(m_player != nullptr, "Map does not contain player");
  PlayerPtr player = std::move(m_player);
  m_player = nullptr;

  return player;
}

void SceneBuilder::createTerrainMaterials()
{
  // TODO: Move this
  MeshFeatureSet meshFeatures{
    .vertexLayout = {
      BufferUsage::AttrPosition,
      BufferUsage::AttrNormal,
      BufferUsage::AttrTexCoord
    },
    .isInstanced = false,
    .isSkybox = false,
    .isAnimated = false,
    .maxInstances = 0
  };
  MaterialFeatureSet materialFeatures{
    .hasTransparency = false,
    .hasTexture = true,
    .hasNormalMap = false
  };

  m_renderSystem.compileShader(meshFeatures, materialFeatures);

  auto groundTexture = loadTexture(m_fileSystem.readFile("resources/textures/ground.png"));
  auto groundMaterial = std::make_unique<Material>(materialFeatures);
  groundMaterial->texture.id = m_renderSystem.addTexture(std::move(groundTexture));
  m_groundMaterial = m_renderSystem.addMaterial(std::move(groundMaterial));

  auto wallTexture = loadTexture(m_fileSystem.readFile("resources/textures/bricks.png"));
  auto wallMaterial = std::make_unique<Material>(materialFeatures);
  wallMaterial->texture.id = m_renderSystem.addTexture(std::move(wallTexture));
  m_wallMaterial = m_renderSystem.addMaterial(std::move(wallMaterial));
}

void SceneBuilder::constructSky()
{
  EntityId entityId = System::nextId();

  auto render = std::make_unique<CRender>(entityId, CRenderType::Skybox);
  auto mesh = cuboid(10000, 10000, 10000, Vec2f{ 1, 1 });
  mesh->featureSet.isSkybox = true;
  uint16_t* indexData = reinterpret_cast<uint16_t*>(mesh->indexBuffer.data.data());
  std::reverse(indexData, indexData + mesh->indexBuffer.data.size() / sizeof(uint16_t));
  std::array<TexturePtr, 6> textures{
    loadTexture(m_fileSystem.readFile("resources/textures/skybox/right.png")),
    loadTexture(m_fileSystem.readFile("resources/textures/skybox/left.png")),
    loadTexture(m_fileSystem.readFile("resources/textures/skybox/top.png")),
    loadTexture(m_fileSystem.readFile("resources/textures/skybox/bottom.png")),
    loadTexture(m_fileSystem.readFile("resources/textures/skybox/front.png")),
    loadTexture(m_fileSystem.readFile("resources/textures/skybox/back.png"))
  };
  auto material = std::make_unique<Material>(MaterialFeatureSet{
    .hasTransparency = false,
    .hasTexture = true,
    .hasNormalMap = false
  });
  material->cubeMap.id = m_renderSystem.addCubeMap(std::move(textures));
  m_renderSystem.compileShader(mesh->featureSet, material->featureSet);
  render->meshes = {
    MeshMaterialPair{
      .mesh = m_renderSystem.addMesh(std::move(mesh)),
      .material = m_renderSystem.addMaterial(std::move(material))
    }
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

    MaterialPtr material = std::make_unique<Material>(MaterialFeatureSet{
      .hasTransparency = false,
      .hasTexture = false,
      .hasNormalMap = false
    });
    material->colour = colour;

    MeshPtr mesh = cuboid(w, h, d, Vec2f{ 1, 1 });
    auto meshId = m_renderSystem.addMesh(std::move(mesh));
    m_renderSystem.compileShader(mesh->featureSet, material->featureSet);
    CRenderPtr render = std::make_unique<CRender>(id, CRenderType::Regular);
    render->meshes = {
      MeshMaterialPair{
        .mesh = meshId,
        .material = m_renderSystem.addMaterial(std::move(material))
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
  float_t yaw = 2.f * static_cast<float>(PI) - atan2(m.at(2, 0), m.at(0, 0));

  float_t x = m.at(0, 3);
  float_t y = m.at(1, 3);
  float_t z = m.at(2, 3);

  m_player = createPlayer(m_renderSystem.camera());
  m_player->rotate(0, yaw);
  m_player->setPosition({ x, y, z });
}

MeshPtr createBottomFace(const std::vector<Vec4f>& points)
{
  MeshPtr mesh = std::make_unique<Mesh>(MeshFeatureSet{
    .vertexLayout = {
      BufferUsage::AttrPosition,
      BufferUsage::AttrNormal,
      BufferUsage::AttrTexCoord
    },
    .isInstanced = false,
    .isSkybox = false,
    .isAnimated = false,
    .maxInstances = 0
  });

  Vec2f textureSize = metresToWorldUnits(Vec2f{ 4, 4 });

  std::vector<Vec3f> positions;
  std::vector<Vec3f> normals;
  std::vector<Vec2f> texCoords;

  for (auto i = points.rbegin(); i != points.rend(); ++i) {
    auto& p = *i;
    positions.push_back(p.sub<3>());
    normals.push_back(Vec3f{ 0, -1, 0 });
    texCoords.push_back(Vec2f{ p[0] / textureSize[0], p[2] / textureSize[1] });
  }

  mesh->attributeBuffers = {
    createBuffer(positions, BufferUsage::AttrPosition),
    createBuffer(normals, BufferUsage::AttrNormal),
    createBuffer(texCoords, BufferUsage::AttrTexCoord)
  };

  mesh->indexBuffer = createBuffer(triangulatePoly(positions), BufferUsage::Index);

  return mesh;
}

MeshPtr createTopFace(const std::vector<Vec4f>& points, float_t height)
{
  auto mesh = createBottomFace(points);
  Vec3f normal{ 0, 1, 0 };

  auto positions = getAttrBufferData<Vec3f>(*mesh, 0, BufferUsage::AttrPosition);
  auto normals = getAttrBufferData<Vec3f>(*mesh, 1, BufferUsage::AttrNormal);
  auto indices = getIndexBufferData(*mesh);

  assert(positions.size() == normals.size());

  for (size_t i = 0; i < positions.size(); ++i) {
    positions[i][1] += height;
    normals[i] = normal;
  }

  std::reverse(indices.begin(), indices.end());

  return mesh;
}

void createSideFaces(Mesh& mesh)
{
  auto positions = fromBytes<Vec3f>(mesh.attributeBuffers[0].data);
  auto normals = fromBytes<Vec3f>(mesh.attributeBuffers[1].data);
  auto texCoords = fromBytes<Vec2f>(mesh.attributeBuffers[2].data);
  auto indices = fromBytes<uint16_t>(mesh.indexBuffer.data);

  Vec2f textureSize = metresToWorldUnits(Vec2f{ 4, 4 });

  assert(positions.size() % 2 == 0);
  size_t n = static_cast<uint16_t>(positions.size() / 2);

  float_t distance = 0.f;
  for (size_t i = 0; i < n; ++i) {
    size_t j = n + i;
    size_t nextI = (i + 1) % n;
    size_t nextJ = n + nextI;

    // Face
    const Vec3f& A = positions[i];
    const Vec3f& B = positions[nextI];
    const Vec3f& C = positions[j];
    const Vec3f& D = positions[nextJ];

    Vec3f normal = -(A - B).cross(A - C).normalise();

    float_t edgeLength = (B - A).magnitude();
    float_t height = C[1] - A[1];

    Vec2f uvA{ distance / textureSize[0], height / textureSize[1] };
    Vec2f uvB{ (distance + edgeLength) / textureSize[0], height / textureSize[1] };
    Vec2f uvC{ distance / textureSize[0], 0 };
    Vec2f uvD{ (distance + edgeLength) / textureSize[0], 0 };

    distance += edgeLength;

    uint16_t idx = static_cast<uint16_t>(positions.size());

    positions.insert(positions.end(), { A, B, C, D });
    normals.insert(normals.end(), { normal, normal, normal, normal });
    texCoords.insert(texCoords.end(), { uvA, uvB, uvC, uvD });

    indices.push_back(idx);      // A
    indices.push_back(idx + 2);  // C
    indices.push_back(idx + 1);  // B

    indices.push_back(idx + 1);  // B
    indices.push_back(idx + 2);  // C
    indices.push_back(idx + 3);  // D
  }

  mesh.attributeBuffers[0].data = toBytes(positions);
  mesh.attributeBuffers[1].data = toBytes(normals);
  mesh.attributeBuffers[2].data = toBytes(texCoords);
  mesh.indexBuffer.data = toBytes(indices);
}

void SceneBuilder::fillArea(const ObjectData& area, const Mat4x4f& transform, float_t height,
  const std::string& entityType)
{
  ObjectData data{};
  data.name = entityType;

  const Vec2f spacing = metresToWorldUnits(Vec2f{ 1, 1 }); // TODO

  auto bounds = computeBounds(area);

  std::vector<Vec2f> perimeter;
  std::transform(area.path.points.begin(), area.path.points.end(), std::back_inserter(perimeter),
    [](const Vec4f& p) { return Vec2f{ p[0], p[2] }; });

  for (float_t x = bounds.first[0]; x <= bounds.second[0]; x += spacing[0]) {
    for (float_t z = bounds.first[1]; z <= bounds.second[1]; z += spacing[1]) {
      if (pointIsInsidePoly(Vec2f{ x, z }, perimeter)) {
        Mat4x4f m = translationMatrix4x4(Vec3f{ x, height, z });
        m_entityFactory.constructEntity(data, transform * m);
      }
    }
  }
}

Mat4x4f SceneBuilder::constructZone(const ObjectData& obj, const Mat4x4f& parentTransform)
{
  ObjectData centredObj = obj;
  centreObject(centredObj);

  EntityId entityId = System::nextId();
  float_t floorHeight = metresToWorldUnits(getFloatValue(centredObj.values, "floor"));
  float_t height = floorHeight;
  auto offset = identityMatrix<float_t, 4>();
  if (floorHeight == 0) {
    height = 1.f;
    offset = translationMatrix4x4(Vec3f{ 0, -height, 0 });
  }

  Mat4x4f transform = parentTransform * offset * centredObj.transform;

  auto bottomFace = createBottomFace(centredObj.path.points);
  auto topFace = createTopFace(centredObj.path.points, height);

  auto mesh = mergeMeshes(*bottomFace, *topFace);
  createSideFaces(*mesh);
  auto positions = getConstAttrBufferData<Vec3f>(*mesh, 0, BufferUsage::AttrPosition);
  float_t radius = computeRadius(positions);

  CRenderPtr render = std::make_unique<CRender>(entityId, CRenderType::Regular);
  render->meshes = {
    MeshMaterialPair{
      .mesh = m_renderSystem.addMesh(std::move(mesh)),
      .material = m_groundMaterial
    }
  };
  m_renderSystem.addComponent(std::move(render));

  CSpatialPtr spatial = std::make_unique<CSpatial>(entityId, transform, radius);
  m_spatialSystem.addComponent(std::move(spatial));

  CCollisionPtr collision = std::make_unique<CCollision>(entityId);
  collision->height = height;
  std::transform(centredObj.path.points.begin(), centredObj.path.points.end(),
    std::back_inserter(collision->perimeter), [](const Vec4f& p) { return Vec2f{ p[0], p[2] }; });
  m_collisionSystem.addComponent(std::move(collision));

  auto i = centredObj.values.find("fill");
  if (i != centredObj.values.end()) {
    fillArea(centredObj, transform, height, i->second);
  }

  return translationMatrix4x4(Vec3f{ 0, floorHeight, 0 }) * obj.transform;
}

void SceneBuilder::constructWall(const ObjectData& obj, const Mat4x4f& parentTransform,
  bool /*interior*/) // TODO: Interiors
{
  const float_t wallThickness = metresToWorldUnits(1.0);
  Vec2f textureSize = metresToWorldUnits(Vec2f{ 4, 4 });

  float_t wallHeight = metresToWorldUnits(getFloatValue(obj.values, "height"));

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

    CRenderPtr render = std::make_unique<CRender>(entityId, CRenderType::Regular);
    auto mesh = cuboid(wallThickness, wallHeight, distance, textureSize);
    auto positions = getConstAttrBufferData<Vec3f>(*mesh, 0, BufferUsage::AttrPosition);
    float_t radius = computeRadius(positions);
    render->meshes = {
      MeshMaterialPair{
        .mesh = m_renderSystem.addMesh(std::move(mesh)),
        .material = m_wallMaterial
      }
    };
    m_renderSystem.addComponent(std::move(render));

    CSpatialPtr spatial = std::make_unique<CSpatial>(entityId,
      parentTransform * obj.transform * m * shift, radius);
    m_spatialSystem.addComponent(std::move(spatial));

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
  const FileSystem& fileSystem, Logger& logger)
{
  SceneBuilder builder(entityFactory, spatialSystem, renderSystem, collisionSystem, mapParser,
    fileSystem, logger);

  return builder.createScene();
}
