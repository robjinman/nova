#pragma once

class SpatialSystem;
class RenderSystem;
class CollisionSystem;
class MapParser;
class Logger;

void createScene(SpatialSystem& spatialSystem, RenderSystem& renderSystem,
  CollisionSystem& collisionSystem, MapParser& mapParser, Logger& logger);
