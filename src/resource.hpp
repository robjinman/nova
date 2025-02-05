#pragma once

#include <string>
#include <memory>

using ResourceId = std::string;

class Resource
{
  public:
    Resource(const ResourceId& id)
      : m_id(id) {}

    const ResourceId& id() const;

    virtual ~Resource() {}

  private:
    ResourceId m_id;
};

using ResourcePtr = std::unique_ptr<Resource>;

const ResourceId& Resource::id() const
{
  return m_id;
}
