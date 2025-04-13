#include "entity_factory.hpp"
#include "logger.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include "units.hpp"
#include "xml.hpp"
#include "spatial_system.hpp"
#include "render_system.hpp"
#include "collision_system.hpp"
#include "file_system.hpp"
#include "map_parser.hpp"
#include <map>
#include <regex>

namespace
{

struct ModelResources
{
  std::vector<MeshMaterialPair> submodels;
};

Vec3f parseVec3f(const std::string s)
{
  static std::regex pattern{
    "(-?\\d+(?:\\.\\d+)?)\\,\\s*(-?\\d+(?:\\.\\d+)?)\\,\\s*(-?\\d+(?:\\.\\d+)?)"};
  std::smatch match;
  std::regex_search(s, match, pattern);

  ASSERT(match.size() == 4, STR("Error parsing Vec3f: " << s));

  return {
    parseFloat<float_t>(match[1].str()),
    parseFloat<float_t>(match[2].str()),
    parseFloat<float_t>(match[3].str())
  };
}

class EntityFactoryImpl : public EntityFactory
{
  public:
    EntityFactoryImpl(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
      CollisionSystem& CollisionSystem, const FileSystem& fileSystem, Logger& logger);

    void loadEntityDefinitions(const XmlNode& entities) override;
    void loadModels(const XmlNode& models) override;
    EntityId constructEntity(const ObjectData& data, const Mat4x4f& transform) const override;

  private:
    Logger& m_logger;
    const FileSystem& m_fileSystem;
    SpatialSystem& m_spatialSystem;
    RenderSystem& m_renderSystem;
    CollisionSystem& m_collisionSystem;

    std::map<std::string, RenderItemId> m_materialResources;
    std::map<std::string, ModelResources> m_models;
    std::map<std::string, XmlNodePtr> m_definitions;

    void loadModel(const std::string& name, bool isInstanced, int maxInstances);
    void constructSpatialComponent(EntityId entityId, const XmlNode& node, const ObjectData& data,
      const Mat4x4f& transform) const;
    void constructRenderComponent(EntityId entityId, const XmlNode& node,
      const ObjectData& data) const;
    ModelResources gpuLoadModel(ModelPtr model);
    MaterialHandle gpuLoadMaterial(MaterialPtr material);
    void constructCollisionComponent(EntityId entityId, const XmlNode& node) const;
    Mat4x4f parseTransform(const XmlNode& node) const;
};

EntityFactoryImpl::EntityFactoryImpl(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, const FileSystem& fileSystem, Logger& logger)
  : m_logger(logger)
  , m_fileSystem(fileSystem)
  , m_spatialSystem(spatialSystem)
  , m_renderSystem(renderSystem)
  , m_collisionSystem(collisionSystem)
{
}

void EntityFactoryImpl::loadEntityDefinitions(const XmlNode& entities)
{
  ASSERT(entities.name() == "entities", "Expected element with name 'entities'");

  for (auto& entity : entities) {
    auto name = entity.attribute("name");

    auto path = STR("entities/" << name << ".xml");
    auto data = m_fileSystem.readFile(path);
    XmlNodePtr root = parseXml(data);

    ASSERT(root->name() == "entity", "Expected 'entity' node");

    m_definitions[name] = std::move(root);
  }
}

void EntityFactoryImpl::loadModels(const XmlNode& models)
{
  ASSERT(models.name() == "models", "Expected element with name 'models'");

  for (auto& model : models) {
    auto name = model.attribute("name");
    bool isInstanced = model.attribute("instanced") == "true";
    int maxInstances = 0;
    if (isInstanced) {
      maxInstances = std::stoi(model.attribute("max-instances"));
    }

    loadModel(name, isInstanced, maxInstances);
  }
}

void EntityFactoryImpl::loadModel(const std::string& name, bool isInstanced, int maxInstances)
{
  auto path = STR("resources/models/" << name << ".gltf");
  auto model = ::loadModel(m_fileSystem, path);

  for (auto& submodel : model->submodels) {
    submodel->mesh->featureSet.isInstanced = isInstanced;
    submodel->mesh->featureSet.maxInstances = maxInstances;
  }

  m_models[name] = gpuLoadModel(std::move(model));
}

ModelResources EntityFactoryImpl::gpuLoadModel(ModelPtr model)
{
  ModelResources resources;

  for (auto& submodel : model->submodels) {
    m_renderSystem.compileShader(submodel->mesh->featureSet, submodel->material->featureSet);

    MeshMaterialPair pair;
    pair.mesh = m_renderSystem.addMesh(std::move(submodel->mesh));
    pair.material = gpuLoadMaterial(std::move(submodel->material));

    resources.submodels.push_back(pair);
  }

  return resources;
}

MaterialHandle EntityFactoryImpl::gpuLoadMaterial(MaterialPtr material)
{
  auto textureFileName = material->texture.fileName;
  if (textureFileName != "") {
    auto texturePath = STR("resources/textures/" << textureFileName);

    auto i = m_materialResources.find(textureFileName);
    if (i == m_materialResources.end()) {
      auto texture = loadTexture(m_fileSystem.readFile(texturePath));
      material->texture.id = m_renderSystem.addTexture(std::move(texture));
      m_materialResources[textureFileName] = material->texture.id;
    }
    else {
      material->texture.id = i->second;
    }
  }

  // TODO: Remove duplication
  auto normalMapFileName = material->normalMap.fileName;
  if (normalMapFileName != "") {
    auto texturePath = STR("resources/textures/" << normalMapFileName);

    auto i = m_materialResources.find(normalMapFileName);
    if (i == m_materialResources.end()) {
      auto texture = loadTexture(m_fileSystem.readFile(texturePath));
      material->normalMap.id = m_renderSystem.addTexture(std::move(texture));
      m_materialResources[normalMapFileName] = material->normalMap.id;
    }
    else {
      material->normalMap.id = i->second;
    }
  }
  // TODO: Repeat the above for cube maps

  return m_renderSystem.addMaterial(std::move(material));
}

EntityId EntityFactoryImpl::constructEntity(const ObjectData& data, const Mat4x4f& transform) const
{
  EntityId id = System::nextId();

  const auto& root = *m_definitions.at(data.name);

  for (auto& node : root) {
    if (node.name() == "spatial-component") {
      constructSpatialComponent(id, node, data, transform);
    }
    else if (node.name() == "render-component") {
      constructRenderComponent(id, node, data);
    }
    else if (node.name() == "collision-component") {
      constructCollisionComponent(id, node);
    }
    // ...
    else {
      EXCEPTION(STR("Unexpected tag '" << node.name() << "'"));
    }
  }

  return id;
}

Mat4x4f EntityFactoryImpl::parseTransform(const XmlNode& node) const
{
  ASSERT(node.name() == "transform", "Expected 'transform' node");

  auto& posNode = *node.child("pos");
  auto& oriNode = *node.child("ori");

  Vec3f pos{
    parseFloat<float_t>(posNode.attribute("x")),
    parseFloat<float_t>(posNode.attribute("y")),
    parseFloat<float_t>(posNode.attribute("z"))
  };

  Vec3f ori{
    degreesToRadians(parseFloat<float_t>(oriNode.attribute("x"))),
    degreesToRadians(parseFloat<float_t>(oriNode.attribute("y"))),
    degreesToRadians(parseFloat<float_t>(oriNode.attribute("z")))
  };

  float_t scale = parseFloat<float_t>(node.attribute("scale"));

  return transform(metresToWorldUnits(pos), ori) * scaleMatrix<float_t, 4>(scale, true);
}

template<typename T>
T getValue(const ObjectData& data, const std::string& name, const T& defaultValue);

template<>
std::string getValue(const ObjectData& data, const std::string& name,
  const std::string& defaultValue)
{
  auto i = data.values.find(name);
  if (i == data.values.end()) {
    return defaultValue;
  }
  return i->second;
}

template<>
float_t getValue(const ObjectData& data, const std::string& name, const float_t& defaultValue)
{
  auto i = data.values.find(name);
  if (i == data.values.end()) {
    return defaultValue;
  }
  return std::stof(i->second);
}

void EntityFactoryImpl::constructSpatialComponent(EntityId entityId, const XmlNode& node,
  const ObjectData& data, const Mat4x4f& transform) const
{
  auto strRadius = node.attribute("radius");
  float_t radius = !strRadius.empty() ? metresToWorldUnits(parseFloat<float_t>(strRadius)) : 0.f;

  auto i = node.child("transform");
  auto m = i != node.end() ? transform * parseTransform(*i) : transform;

  float_t height = metresToWorldUnits(getValue<float_t>(data, "height", 0));
  m.set(1, 3, m.at(1, 3) + height);

  auto spatial = std::make_unique<CSpatial>(entityId, m, radius);

  m_spatialSystem.addComponent(std::move(spatial));
}

CRenderType parseCRenderType(const std::string& type)
{
  if (type == "regular") return CRenderType::Regular;
  else if (type == "instance") return CRenderType::Instance;
  else if (type == "light") return CRenderType::Light;
  else if (type == "skybox") return CRenderType::Skybox;
  else EXCEPTION(STR("Unrecognised render component type '" << type << "'"));
}

void EntityFactoryImpl::constructRenderComponent(EntityId entityId, const XmlNode& node,
  const ObjectData& data) const
{
  auto type = parseCRenderType(node.attribute("type"));

  CRenderPtr render = nullptr;
  if (type == CRenderType::Light) {
    auto light = std::make_unique<CRenderLight>(entityId);
    light->colour = parseVec3f(data.values.at("colour"));
    light->ambient = parseFloat<float_t>(data.values.at("ambient"));
    light->specular = parseFloat<float_t>(data.values.at("specular"));
    render = std::move(light);
  }
  else {
    render = std::make_unique<CRender>(entityId, type);
  }

  auto model = node.attribute("model");
  if (!model.empty()) {
    auto& resources = m_models.at(model);

    for (auto& submodel : resources.submodels) {
      render->meshes.push_back(submodel);
    }
  }

  m_renderSystem.addComponent(std::move(render));
}

void EntityFactoryImpl::constructCollisionComponent(EntityId entityId, const XmlNode& node) const
{
  auto collision = std::make_unique<CCollision>(entityId);

  // Units in model space

  collision->height = parseFloat<float_t>(node.attribute("height"));

  for (auto& pointNode : node) {
    ASSERT(pointNode.name() == "point", "Expected point node");

    collision->perimeter.push_back(Vec2f{
      parseFloat<float_t>(pointNode.attribute("x")),
      parseFloat<float_t>(pointNode.attribute("y"))
    });
  }

  m_collisionSystem.addComponent(std::move(collision));
}

}

EntityFactoryPtr createEntityFactory(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, const FileSystem& fileSystem, Logger& logger)
{
  return std::make_unique<EntityFactoryImpl>(spatialSystem, renderSystem, collisionSystem,
    fileSystem, logger);
}
