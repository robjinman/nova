#pragma once

#include "player.hpp"
#include <memory>

class EntityFactory;
class SpatialSystem;
class RenderSystem;
class CollisionSystem;
class MapParser;
class FileSystem;
class Logger;

PlayerPtr createScene(EntityFactory& entityFactory, SpatialSystem& spatialSystem,
  RenderSystem& renderSystem, CollisionSystem& collisionSystem, MapParser& mapParser,
  const FileSystem& fileSystem, Logger& logger);
