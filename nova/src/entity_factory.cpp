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
#include <map>

namespace
{

class EntityFactoryImpl : public EntityFactory
{
  public:
    EntityFactoryImpl(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
      CollisionSystem& CollisionSystem, const FileSystem& fileSystem, Logger& logger);

    EntityId constructEntity(const std::string& name, const Mat4x4f& transform) const override;

  private:
    Logger& m_logger;
    const FileSystem& m_fileSystem;
    SpatialSystem& m_spatialSystem;
    RenderSystem& m_renderSystem;
    CollisionSystem& m_collisionSystem;

    std::map<std::string, RenderItemId> m_textures;
    std::map<std::string, RenderItemId> m_meshes;
    std::map<std::string, RenderItemId> m_materials;
    std::map<std::string, XmlNodePtr> m_definitions;

    void loadResources();
    void loadTextures(const XmlNode& node);
    void loadMeshes(const XmlNode& node);
    void loadMaterials(const XmlNode& node);
    void loadEntities();
    void parseEntityFile(const std::filesystem::path& path);
    void constructSpatialComponent(EntityId entityId, const XmlNode& node,
      const Mat4x4f& transform) const;
    void constructRenderComponent(EntityId entityId, const XmlNode& node) const;
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
  loadResources(); // TODO: Move to resource manager?
  loadEntities();
}

void EntityFactoryImpl::loadResources()
{
  XmlNodePtr root = openXmlFile("data/resources/manifest.xml");
  ASSERT(root->name() == "resources", "Expected root node 'resources'");

  for (auto& node : *root) {
    if (node.name() == "textures") {
      loadTextures(node);
    }
    else if (node.name() == "meshes") {
      loadMeshes(node);
    }
    else if (node.name() == "materials") {
      loadMaterials(node);
    }
    // ...
  }
}

void EntityFactoryImpl::loadTextures(const XmlNode& root)
{
  for (auto& node : root) {
    ASSERT(node.name() == "texture", "Expected 'texture' node");
    m_textures[node.attribute("name")] =
      m_renderSystem.addTexture(loadTexture(std::string("data/") + node.attribute("file")));
  }
}

void EntityFactoryImpl::loadMeshes(const XmlNode& root)
{
  for (auto& node : root) {
    ASSERT(node.name() == "mesh", "Expected 'mesh' node");
    auto mesh = loadMesh(std::string("data/") + node.attribute("file"));
    mesh->isInstanced = node.attribute("instanced") == "true";
    if (mesh->isInstanced) {
      mesh->maxInstances = std::stoi(node.attribute("max-instances"));
    }
    m_meshes[node.attribute("name")] = m_renderSystem.addMesh(std::move(mesh));
  }
}

void EntityFactoryImpl::loadMaterials(const XmlNode& root)
{
  for (auto& node : root) {
    ASSERT(node.name() == "material", "Expected 'material' node");
    auto material = std::make_unique<Material>();
    material->texture = m_textures.at(node.attribute("texture"));
    auto normalMap = node.attribute("normal-map");
    if (!normalMap.empty()) {
      material->normalMap = m_textures.at(node.attribute(normalMap));
    }
    m_materials[node.attribute("name")] = m_renderSystem.addMaterial(std::move(material));
  }
}

void EntityFactoryImpl::loadEntities()
{
  auto directory = m_fileSystem.directory("entities");
  for (auto file : *directory) {
    parseEntityFile(file);
  }
}

void EntityFactoryImpl::parseEntityFile(const std::filesystem::path& path)
{
  XmlNodePtr root = openXmlFile(path.string());
  ASSERT(root->name() == "entity", "Expected root node 'entity'");

  std::string entityName = root->attribute("name");
  m_definitions[entityName] = std::move(root);
}

EntityId EntityFactoryImpl::constructEntity(const std::string& name, const Mat4x4f& transform) const
{
  EntityId id = System::nextId();

  const auto& root = *m_definitions.at(name);

  for (auto& node : root) {
    if (node.name() == "spatial-component") {
      constructSpatialComponent(id, node, transform);
    }
    else if (node.name() == "render-component") {
      constructRenderComponent(id, node);
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

void EntityFactoryImpl::constructSpatialComponent(EntityId entityId, const XmlNode& node,
  const Mat4x4f& transform) const
{
  auto spatial = std::make_unique<CSpatial>(entityId);
  auto& transformNode = *node.child("transform");
  spatial->setTransform(transform * parseTransform(transformNode));
  m_spatialSystem.addComponent(std::move(spatial));
}

CRenderType parseCRenderType(const std::string& type)
{
  if (type == "regular")        return CRenderType::regular;
  else if (type == "instance")  return CRenderType::instance;
  else if (type == "skybox")    return CRenderType::skybox;
  else EXCEPTION(STR("Unrecognised render component type '" << type << "'"));
}

void EntityFactoryImpl::constructRenderComponent(EntityId entityId, const XmlNode& node) const
{
  auto type = parseCRenderType(node.attribute("type"));

  auto render = std::make_unique<CRender>(entityId, type);
  render->mesh = m_meshes.at(node.attribute("mesh"));
  auto material = node.attribute("material");
  if (!material.empty()) {
    render->material = m_materials.at(material);
  }

  m_renderSystem.addComponent(std::move(render));
}

void EntityFactoryImpl::constructCollisionComponent(EntityId entityId, const XmlNode& node) const
{
  auto collision = std::make_unique<CCollision>(entityId);

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
