#include "system.hpp"

EntityId System::m_nextId = 0;

EntityId System::nextId()
{
  return m_nextId++;
}

Component::Component(EntityId entityId)
  : m_id(entityId)
{
}

EntityId Component::id() const
{
  return m_id;
}

Component::~Component()
{
}
