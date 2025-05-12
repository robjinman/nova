#pragma once

#include "system.hpp"
#include "math.hpp"
#include <unordered_set>
#include <memory>

class CSpatial : public Component
{
  public:
    CSpatial(EntityId entityId, const Mat4x4f& transform, float_t radius);

    const Mat4x4f& relTransform() const;
    const Mat4x4f& absTransform() const;
    float_t radius() const;

  private:
    EntityId m_parent;
    Mat4x4f m_transform;
    float_t m_radius;
    std::unordered_set<EntityId> m_children;
};

using CSpatialPtr = std::unique_ptr<CSpatial>;

// TODO: Provide efficient way to traverse visible entities, computing absolute transforms during
// traversal
class SpatialSystem : public System
{
  public:
    CSpatial& getComponent(EntityId id) override = 0;
    const CSpatial& getComponent(EntityId id) const override = 0;

    virtual std::unordered_set<EntityId> getIntersecting(const std::vector<Vec2f>& poly) const = 0;

    virtual ~SpatialSystem() {}
};

using SpatialSystemPtr = std::unique_ptr<SpatialSystem>;

class Logger;

SpatialSystemPtr createSpatialSystem(Logger& logger);
