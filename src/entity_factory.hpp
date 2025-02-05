#pragma once

#include "system.hpp"
#include "math.hpp"
#include <memory>
#include <string>

class EntityFactory
{
  public:
    virtual EntityId constructEntity(const std::string& name, const Mat4x4f& transform) const = 0;

    virtual ~EntityFactory() {}
};

using EntityFactoryPtr = std::unique_ptr<EntityFactory>;

class Logger;
class SpatialSystem;
class RenderSystem;
class CollisionSystem;

EntityFactoryPtr createEntityFactory(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, Logger& logger);
