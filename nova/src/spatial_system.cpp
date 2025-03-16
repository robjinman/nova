#include "spatial_system.hpp"
#include "logger.hpp"
#include "grid.hpp"
#include <map>

CSpatial::CSpatial(EntityId entityId, const Mat4x4f& transform, float_t radius)
  : Component(entityId)
  , m_transform(transform)
  , m_radius(radius)
{
}

const Mat4x4f& CSpatial::relTransform() const
{
  return m_transform;
}

const Mat4x4f& CSpatial::absTransform() const
{
  // TODO
  return m_transform;
}

float_t CSpatial::radius() const
{
  return m_radius;
}

namespace
{

class SpatialSystemImpl : public SpatialSystem
{
  public:
    SpatialSystemImpl(Logger& logger);

    void addComponent(ComponentPtr component) override;
    void removeComponent(EntityId entityId) override;
    bool hasComponent(EntityId entityId) const override;
    CSpatial& getComponent(EntityId entityId) override;
    const CSpatial& getComponent(EntityId entityId) const override;
    void update() override;

    std::unordered_set<EntityId> getIntersecting(const std::vector<Vec2f>& poly) const override;

  private:
    Logger& m_logger;
    std::map<EntityId, CSpatialPtr> m_components;
    Grid<EntityId, 100, 100> m_grid;
};

} // namespace

SpatialSystemImpl::SpatialSystemImpl(Logger& logger)
  : m_logger(logger)
  , m_grid(Vec2f{-400.f, -400.f}, Vec2f{1200.f, 1200.f}) // TODO
{
}

void SpatialSystemImpl::addComponent(ComponentPtr component)
{
  auto spatial = CSpatialPtr(dynamic_cast<CSpatial*>(component.release()));
  Vec3f pos = getTranslation(spatial->absTransform());
  m_grid.addItemByRadius(Vec2f{ pos[0], pos[2] }, spatial->radius(), spatial->id()); 
  m_components[spatial->id()] = std::move(spatial);
}

void SpatialSystemImpl::removeComponent(EntityId entityId)
{
  m_components.erase(entityId);
}

bool SpatialSystemImpl::hasComponent(EntityId entityId) const
{
  return m_components.count(entityId) > 0;
}

CSpatial& SpatialSystemImpl::getComponent(EntityId entityId)
{
  return *m_components.at(entityId);
}

const CSpatial& SpatialSystemImpl::getComponent(EntityId entityId) const
{
  return *m_components.at(entityId);
}

void SpatialSystemImpl::update()
{
}

std::unordered_set<EntityId>
SpatialSystemImpl::getIntersecting(const std::vector<Vec2f>& poly) const
{
  return m_grid.getItems(poly);
}

SpatialSystemPtr createSpatialSystem(Logger& logger)
{
  return std::make_unique<SpatialSystemImpl>(logger);
}
