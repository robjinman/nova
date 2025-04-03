#pragma once

#include "system.hpp"
#include "math.hpp"
#include <memory>
#include <string>

class XmlNode;
struct ObjectData;

class EntityFactory
{
  public:
    virtual void loadEntityDefinitions(const XmlNode& entities) = 0;
    virtual void loadModels(const XmlNode& models) = 0;
    virtual EntityId constructEntity(const ObjectData& data, const Mat4x4f& transform) const = 0;

    virtual ~EntityFactory() {}
};

using EntityFactoryPtr = std::unique_ptr<EntityFactory>;

class SpatialSystem;
class RenderSystem;
class CollisionSystem;
class FileSystem;
class Logger;

EntityFactoryPtr createEntityFactory(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, const FileSystem& fileSystem, Logger& logger);
