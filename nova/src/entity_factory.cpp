#include "entity_factory.hpp"
#include "logger.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include "units.hpp"
#include "xml.hpp"
#include "spatial_system.hpp"
#include "model_loader.hpp"
#include "collision_system.hpp"
#include "file_system.hpp"
#include "map_parser.hpp"
#include <map>
#include <regex>

using render::Material;
using render::MaterialPtr;
namespace MeshFeatures = render::MeshFeatures;
namespace MaterialFeatures = render::MaterialFeatures;
using render::MaterialFeatureSet;
using render::BufferUsage;

namespace
{

struct MaterialCustomisation
{
  bool hasTransparency = false;
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

std::string getStringValue(const ObjectData& data, const std::string& name,
  const std::string& defaultValue)
{
  auto i = data.values.find(name);
  if (i == data.values.end()) {
    return defaultValue;
  }
  return i->second;
}

float_t getFloatValue(const ObjectData& data, const std::string& name, const float_t& defaultValue)
{
  auto i = data.values.find(name);
  if (i == data.values.end()) {
    return defaultValue;
  }
  return std::stof(i->second);
}

void customiseMaterial(Material& material, const MaterialCustomisation& props)
{
  material.featureSet.flags.set(MaterialFeatures::HasTransparency, props.hasTransparency);
}

class EntityFactoryImpl : public EntityFactory
{
  public:
    EntityFactoryImpl(ModelLoader& modelLoader, SpatialSystem& spatialSystem,
      RenderSystem& renderSystem, CollisionSystem& CollisionSystem, const FileSystem& fileSystem,
      Logger& logger);

    void loadEntityDefinitions(const XmlNode& entities) override;
    void loadMaterials(const XmlNode& materials) override;
    void loadModels(const XmlNode& models) override;
    EntityId constructEntity(const ObjectData& data, const Mat4x4f& transform) const override;

  private:
    Logger& m_logger;
    ModelLoader& m_modelLoader;
    const FileSystem& m_fileSystem;
    SpatialSystem& m_spatialSystem;
    RenderSystem& m_renderSystem;
    CollisionSystem& m_collisionSystem;

    std::map<std::string, CRenderModelPtr> m_renderComponents;
    std::map<std::string, XmlNodePtr> m_definitions;
    std::map<std::string, MaterialCustomisation> m_materialProperties;

    void constructSpatialComponent(EntityId entityId, const XmlNode& node, const ObjectData& data,
      const Mat4x4f& transform) const;
    void constructRenderComponent(EntityId entityId, const XmlNode& node,
      const ObjectData& data) const;
    void constructCollisionComponent(EntityId entityId, const XmlNode& node) const;
    Mat4x4f parseTransform(const XmlNode& node) const;
};

EntityFactoryImpl::EntityFactoryImpl(ModelLoader& modelLoader, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, const FileSystem& fileSystem,
  Logger& logger)
  : m_logger(logger)
  , m_modelLoader(modelLoader)
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

void EntityFactoryImpl::loadMaterials(const XmlNode& materials)
{
  ASSERT(materials.name() == "materials", "Expected element with name 'materials'");

  for (auto& material : materials) {
    std::string name = material.attribute("name");
    MaterialCustomisation props{
      .hasTransparency = material.attribute("has-transparency") == "true"
    };
    m_materialProperties.insert(std::make_pair(name, props));
  }
}

void EntityFactoryImpl::loadModels(const XmlNode& modelsData)
{
  ASSERT(modelsData.name() == "models", "Expected element with name 'models'");

  for (auto& modelData : modelsData) {
    auto name = modelData.attribute("name");
    bool isInstanced = modelData.attribute("instanced") == "true";
    int maxInstances = 0;
    if (isInstanced) {
      maxInstances = std::stoi(modelData.attribute("max-instances"));
    }
    bool castsShadow = modelData.attribute("casts-shadow") == "true";

    auto path = STR("resources/models/" << name << ".gltf");
    auto model = m_modelLoader.loadModelData(path);
  
    for (auto& submodel : model->submodels) {
      submodel->mesh->featureSet.flags.set(MeshFeatures::IsInstanced, isInstanced);
      submodel->mesh->featureSet.flags.set(MeshFeatures::CastsShadow, castsShadow);

      submodel->mesh->maxInstances = maxInstances;
  
      auto i = m_materialProperties.find(submodel->material->name);
      if (i != m_materialProperties.end()) {
        customiseMaterial(*submodel->material, i->second);
      }
    }

    m_renderComponents[name] = m_modelLoader.createRenderComponent(std::move(model), isInstanced);
  }
}

EntityId EntityFactoryImpl::constructEntity(const ObjectData& data, const Mat4x4f& transform) const
{
  std::string name = getStringValue(data, "name", "");
  EntityId id = name.empty() ? System::nextId() : System::idFromString(name);

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

  return createTransform(metresToWorldUnits(pos), ori) * scaleMatrix<float_t, 4>(scale, true);
}

void EntityFactoryImpl::constructSpatialComponent(EntityId entityId, const XmlNode& node,
  const ObjectData& data, const Mat4x4f& fromMapTransform) const
{
  auto strRadius = node.attribute("radius");
  float_t radius = !strRadius.empty() ? metresToWorldUnits(parseFloat<float_t>(strRadius)) : 0.f;

  auto i = node.child("transform");
  auto typeTransform = i != node.end() ? parseTransform(*i) : identityMatrix<float_t, 4>();

  float_t height = metresToWorldUnits(getFloatValue(data, "height", 0));
  float_t tilt = degreesToRadians(getFloatValue(data, "tilt", 0));
  auto instanceTransform = createTransform(Vec3f{ 0, height, 0 }, Vec3f{ tilt, 0, 0 });

  auto transform = fromMapTransform * instanceTransform * typeTransform;

  auto spatial = std::make_unique<CSpatial>(entityId, transform, radius);

  m_spatialSystem.addComponent(std::move(spatial));
}

CRenderType parseCRenderType(const std::string& type)
{
  if (type == "model") return CRenderType::Model;
  else if (type == "light") return CRenderType::Light;
  else if (type == "skybox") return CRenderType::Skybox;
  // TODO: Particle emitter
  else EXCEPTION(STR("Unrecognised render component type '" << type << "'"));
}

void EntityFactoryImpl::constructRenderComponent(EntityId entityId, const XmlNode& node,
  const ObjectData& data) const
{
  auto type = parseCRenderType(node.attribute("type"));

  CRenderPtr render = nullptr;
  if (type == CRenderType::Light) {
    float_t size = metresToWorldUnits(0.5);

    auto light = std::make_unique<CRenderLight>(entityId);
    auto colour = parseVec3f(data.values.at("colour"));
    light->colour = colour;
    light->ambient = parseFloat<float_t>(data.values.at("ambient"));
    light->specular = parseFloat<float_t>(data.values.at("specular"));

    auto mesh = render::cuboid(size, size, size, {});
    mesh->attributeBuffers.resize(2);
    mesh->featureSet.vertexLayout = { BufferUsage::AttrPosition, BufferUsage::AttrNormal };
    mesh->featureSet.flags.set(MeshFeatures::CastsShadow, false);
    MaterialPtr material = std::make_unique<Material>(MaterialFeatureSet{});
    material->colour = { colour[0], colour[1], colour[2], 1.f };

    light->submodels.push_back(Submodel{
      .mesh = m_renderSystem.addMesh(std::move(mesh)),
      .material = m_renderSystem.addMaterial(std::move(material)),
      .skin = {}
    });

    render = std::move(light);
  }
  else {
    auto modelName = node.attribute("model");
    if (!modelName.empty()) {
      auto& prototype = *m_renderComponents.at(modelName);
      render = std::make_unique<CRenderModel>(prototype, entityId);
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

EntityFactoryPtr createEntityFactory(ModelLoader& modelLoader, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, const FileSystem& fileSystem,
  Logger& logger)
{
  return std::make_unique<EntityFactoryImpl>(modelLoader, spatialSystem, renderSystem,
    collisionSystem, fileSystem, logger);
}
