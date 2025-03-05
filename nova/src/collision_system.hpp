#pragma once

#include "math.hpp"
#include "system.hpp"
#include <vector>
#include <memory>

struct CCollision : public Component
{
  CCollision(EntityId entityId);
  CCollision(EntityId entityId, const CCollision& cpy);

  std::vector<Vec2f> perimeter;
  float_t height;
};

using CCollisionPtr = std::unique_ptr<CCollision>;

class CollisionSystem : public System
{
  public:
    virtual void initialise(const Vec2f& worldMin, const Vec2f& worldMax) = 0; // TODO

    virtual Vec3f tryMove(const Vec3f& pos, const Vec3f& delta, float_t radius,
      float_t stepHeight) const = 0;

    virtual float_t altitude(const Vec3f& pos) const = 0;

    virtual ~CollisionSystem() {}
};

using CollisionSystemPtr = std::unique_ptr<CollisionSystem>;

class SpatialSystem;
class Logger;

CollisionSystemPtr createCollisionSystem(const SpatialSystem& spatialSystem, Logger& logger);
