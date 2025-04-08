#pragma once

#include "math.hpp"

struct ObjectData;

class Terrain
{
  public:
    virtual Mat4x4f constructZone(const ObjectData& obj, const Mat4x4f& parentTransform) = 0;
    virtual void constructWall(const ObjectData& obj, const Mat4x4f& parentTransform,
      bool interior) = 0;

    virtual ~Terrain() {}
};

using TerrainPtr = std::unique_ptr<Terrain>;

class EntityFactory;
class SpatialSystem;
class RenderSystem;
class CollisionSystem;
class FileSystem;
class Logger;

TerrainPtr createTerrain(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, const FileSystem& fileSystem,
  Logger& logger);
