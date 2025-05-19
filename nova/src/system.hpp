#pragma once

#include <memory>
#include <set>
#include <string>

using EntityId = size_t;

class Component
{
  public:
    Component(EntityId entityId);

    EntityId id() const;

    virtual ~Component() = 0;

  protected:
    EntityId m_id;
};

using ComponentPtr = std::unique_ptr<Component>;

class System
{
  public:
    virtual void addComponent(ComponentPtr component) = 0;
    virtual void removeComponent(EntityId entityId) = 0;
    virtual bool hasComponent(EntityId entityId) const = 0;
    virtual Component& getComponent(EntityId entityId) = 0;
    virtual const Component& getComponent(EntityId entityId) const = 0;
    virtual void update() = 0;

    virtual ~System() {}

    static EntityId idFromString(const std::string& name);
    static EntityId nextId();

  private:
    static EntityId m_nextId;
    static std::set<EntityId> m_reservedIds;
};

using SystemPtr = std::unique_ptr<System>;
