#include "xml.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include <tinyxml2.h>
#include <map>

using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;
using tinyxml2::XMLAttribute;
using tinyxml2::XMLError;

XmlNode::Iterator::Iterator(std::vector<std::unique_ptr<XmlNode>>::const_iterator i)
  : m_i(i)
{
}

bool XmlNode::Iterator::operator==(const Iterator& rhs) const
{
  return m_i == rhs.m_i;
}

bool XmlNode::Iterator::operator!=(const Iterator& rhs) const
{
  return !(*this == rhs);
}

const XmlNode& XmlNode::Iterator::operator*() const
{
  return **m_i;
}

const XmlNode* XmlNode::Iterator::operator->() const
{
  return m_i->get();
}

XmlNode::Iterator& XmlNode::Iterator::operator++()
{
  next();
  return *this;
}

void XmlNode::Iterator::next()
{
  ++m_i;
}

namespace
{

class XmlNodeImpl : public XmlNode
{
  public:
    XmlNodeImpl(const XMLElement& node);

    const std::string& name() const override;
    const std::string& contents() const override;
    Iterator child(const std::string& name) const override;
    std::string attribute(const std::string& name) const override;
    Iterator begin() const override;
    Iterator end() const override;

  private:
    std::string m_name;
    std::string m_contents;
    std::map<std::string, std::string> m_attributes;
    std::vector<std::unique_ptr<XmlNode>> m_children;
};

XmlNodeImpl::XmlNodeImpl(const XMLElement& node)
{
  m_name = node.Name();
  auto contents = node.GetText();
  if (contents != nullptr) {
    m_contents = contents;
  }

  for (auto attr = node.FirstAttribute(); attr != nullptr; attr = attr->Next()) {
    std::string attrName = attr->Name();
    std::string attrValue = attr->Value();
    m_attributes[attrName] = attrValue;
  }

  for (auto elem = node.FirstChildElement(); elem != nullptr; elem = elem->NextSiblingElement()) {
    std::string name = elem->Name();
    m_children.push_back(std::make_unique<XmlNodeImpl>(*elem));
  }
}

const std::string& XmlNodeImpl::name() const
{
  return m_name;
}

const std::string& XmlNodeImpl::contents() const
{
  return m_contents;
}

XmlNode::Iterator XmlNodeImpl::child(const std::string& name) const
{
  for (auto i = m_children.begin(); i != m_children.end(); ++i) {
    if ((*i)->name() == name) {
      return i;
    }
  }

  return end();
}

std::string XmlNodeImpl::attribute(const std::string& name) const
{
  auto i = m_attributes.find(name);
  return i != m_attributes.end() ? i->second : "";
}

XmlNode::Iterator XmlNodeImpl::begin() const
{
  return Iterator(m_children.begin());
}

XmlNode::Iterator XmlNodeImpl::end() const
{
  return Iterator(m_children.end());
}

} // namespace

XmlNodePtr parseXml(const std::vector<char>& data)
{
  XMLDocument doc;
  if (doc.Parse(data.data(), data.size()) != XMLError::XML_SUCCESS) {
    EXCEPTION("Error parsing XML");
  }

  return std::make_unique<XmlNodeImpl>(*doc.RootElement());
}
