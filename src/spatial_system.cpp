#include "spatial_system.hpp"
#include "logger.hpp"
#include <map>

CSpatial::CSpatial(EntityId entityId)
  : Component(entityId)
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

void CSpatial::setTransform(const Mat4x4f& transform)
{
  m_transform = transform;
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

  private:
    Logger& m_logger;
    std::map<EntityId, CSpatialPtr> m_components;
};

} // namespace

SpatialSystemImpl::SpatialSystemImpl(Logger& logger)
  : m_logger(logger)
{
}

void SpatialSystemImpl::addComponent(ComponentPtr component)
{
  auto spatialComp = CSpatialPtr(dynamic_cast<CSpatial*>(component.release()));
  m_components[spatialComp->id()] = std::move(spatialComp);
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

SpatialSystemPtr createSpatialSystem(Logger& logger)
{
  return std::make_unique<SpatialSystemImpl>(logger);
}
