#include "terrain.hpp"
#include "entity_factory.hpp"
#include "spatial_system.hpp"
#include "render_system.hpp"
#include "collision_system.hpp"
#include "logger.hpp"
#include "map_parser.hpp"
#include "units.hpp"
#include "utils.hpp"
#include "file_system.hpp"
#include <cassert>
#include <random>

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

MeshPtr createBottomFace(const std::vector<Vec4f>& points, const MeshFeatureSet& meshFeatures)
{
  MeshPtr mesh = std::make_unique<Mesh>(meshFeatures);

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

MeshPtr createTopFace(const std::vector<Vec4f>& points, float_t height,
  const MeshFeatureSet& meshFeatures)
{
  auto mesh = createBottomFace(points, meshFeatures);
  Vec3f normal{ 0, 1, 0 };

  auto positions = getBufferData<Vec3f>(mesh->attributeBuffers[0]);
  auto normals = getBufferData<Vec3f>(mesh->attributeBuffers[1]);
  auto indices = getIndexBufferData(*mesh);

  assert(positions.size() == normals.size());

  for (size_t i = 0; i < positions.size(); ++i) {
    positions[i][1] += height;
    normals[i] = normal;
  }

  std::reverse(indices.begin(), indices.end());

  return mesh;
}

class TerrainImpl : public Terrain
{
  public:
    TerrainImpl(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
      RenderSystem& renderSystem, CollisionSystem& collisionSystem, const FileSystem& fileSystem,
      Logger& logger);

    Mat4x4f constructZone(const ObjectData& obj, const Mat4x4f& parentTransform) override;
    void constructWall(const ObjectData& obj, const Mat4x4f& parentTransform,
      bool interior) override;

  private:
    Logger& m_logger;
    const FileSystem& m_fileSystem;
    EntityFactory& m_entityFactory;
    SpatialSystem& m_spatialSystem;
    RenderSystem& m_renderSystem;
    CollisionSystem& m_collisionSystem;
    MaterialHandle m_groundMaterial;
    MaterialHandle m_wallMaterial;
    MeshFeatureSet m_meshFeatures;

    void createTerrainMaterials();
    void fillArea(const ObjectData& area, const Mat4x4f& transform, float_t height,
      const std::string& entityType);
};

TerrainImpl::TerrainImpl(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, const FileSystem& fileSystem,
  Logger& logger)
  : m_logger(logger)
  , m_fileSystem(fileSystem)
  , m_entityFactory(entityFactory)
  , m_spatialSystem(spatialSystem)
  , m_renderSystem(renderSystem)
  , m_collisionSystem(collisionSystem)
{
  MeshFeatures::Flags flags{};
  flags.set(MeshFeatures::CastsShadow);

  m_meshFeatures = MeshFeatureSet{
    .vertexLayout = {
      BufferUsage::AttrPosition,
      BufferUsage::AttrNormal,
      BufferUsage::AttrTexCoord
    },
    .flags = flags
  };

  createTerrainMaterials();
}

void TerrainImpl::fillArea(const ObjectData& area, const Mat4x4f& transform, float_t height,
  const std::string& entityType)
{
  ObjectData data{};
  data.name = entityType;

  // TODO: Read spacing from object data
  const Vec2f spacing = metresToWorldUnits(Vec2f{ 1, 1 });

  auto bounds = computeBounds(area);

  std::mt19937 randomEngine;
  std::uniform_real_distribution<float_t> uniformDistribution{0, 1};

  std::vector<Vec2f> perimeter;
  std::transform(area.path.points.begin(), area.path.points.end(), std::back_inserter(perimeter),
    [](const Vec4f& p) { return Vec2f{ p[0], p[2] }; });

  for (float_t x = bounds.first[0]; x <= bounds.second[0]; x += spacing[0]) {
    for (float_t z = bounds.first[1]; z <= bounds.second[1]; z += spacing[1]) {
      if (pointIsInsidePoly(Vec2f{ x, z }, perimeter)) {
        // TODO: Read transform from object data
        float_t rot = uniformDistribution(randomEngine) * 2.f * PIf;
        Mat4x4f m = createTransform(Vec3f{ x, height, z }, Vec3f{ 0, rot, 0 });

        m_entityFactory.constructEntity(data, transform * m);
      }
    }
  }
}

void TerrainImpl::createTerrainMaterials()
{
  MeshFeatures::Flags meshFlags{};
  meshFlags.set(MeshFeatures::CastsShadow);

  MaterialFeatures::Flags materialFlags{};
  materialFlags.set(MaterialFeatures::HasTexture);

  MeshFeatureSet meshFeatures{
    .vertexLayout = {
      BufferUsage::AttrPosition,
      BufferUsage::AttrNormal,
      BufferUsage::AttrTexCoord
    },
    .flags = meshFlags
  };
  MaterialFeatureSet materialFeatures{
    .flags = materialFlags
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

void TerrainImpl::constructWall(const ObjectData& obj, const Mat4x4f& parentTransform,
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
    mesh->featureSet = m_meshFeatures;
    auto positions = getConstBufferData<Vec3f>(mesh->attributeBuffers[0]);
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

Mat4x4f TerrainImpl::constructZone(const ObjectData& obj, const Mat4x4f& parentTransform)
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

  auto bottomFace = createBottomFace(centredObj.path.points, m_meshFeatures);
  auto topFace = createTopFace(centredObj.path.points, height, m_meshFeatures);

  auto mesh = mergeMeshes(*bottomFace, *topFace);
  createSideFaces(*mesh);
  auto positions = getConstBufferData<Vec3f>(mesh->attributeBuffers[0]);
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

} // namespace

TerrainPtr createTerrain(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, const FileSystem& fileSystem,
  Logger& logger)
{
  return std::make_unique<TerrainImpl>(entityFactory, spatialSystem, renderSystem, collisionSystem,
    fileSystem, logger);
}
