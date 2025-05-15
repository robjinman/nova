#include "system.hpp"

EntityId System::m_nextId = 0;
std::set<EntityId> System::m_reservedIds;

EntityId System::nextId()
{
  while (m_reservedIds.contains(m_nextId)) {
    ++m_nextId;
  }
  return m_nextId++;
}

EntityId System::idFromString(const std::string& name)
{
  EntityId id = std::hash<std::string>{}(name);
  m_reservedIds.insert(id);
  return id;
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
