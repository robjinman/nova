#pragma once

#include "math.hpp"
#include "system.hpp"
#include <set>
#include <memory>

class CSpatial : public Component
{
  public:
    CSpatial(EntityId entityId);

    const Mat4x4f& relTransform() const;
    const Mat4x4f& absTransform() const;

    void setTransform(const Mat4x4f& transform);

  private:
    EntityId m_parent;
    Mat4x4f m_transform = identityMatrix<float_t, 4>();
    std::set<EntityId> m_children;
};

using CSpatialPtr = std::unique_ptr<CSpatial>;

class SpatialSystem : public System
{
  public:
    virtual CSpatial& getComponent(EntityId id) override = 0;
    virtual const CSpatial& getComponent(EntityId id) const override = 0;

    virtual ~SpatialSystem() {}
};

using SpatialSystemPtr = std::unique_ptr<SpatialSystem>;

class Logger;

SpatialSystemPtr createSpatialSystem(Logger& logger);
