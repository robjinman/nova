#pragma once

#include <string>
#include <memory>
#include <vector>

class XmlNode
{
  public:
    class Iterator
    {
      public:
        Iterator(std::vector<std::unique_ptr<XmlNode>>::const_iterator i);

        bool operator==(const Iterator& rhs) const;
        bool operator!=(const Iterator& rhs) const;
        const XmlNode& operator*() const;
        const XmlNode* operator->() const;
        Iterator& operator++();
        void next();

      private:
        std::vector<std::unique_ptr<XmlNode>>::const_iterator m_i;
    };

    virtual const std::string& name() const = 0;
    virtual const std::string& contents() const = 0;
    virtual Iterator child(const std::string& name) const = 0;
    virtual std::string attribute(const std::string& name) const = 0;
    virtual Iterator begin() const = 0;
    virtual Iterator end() const = 0;

    virtual ~XmlNode() {}
};

using XmlNodePtr = std::unique_ptr<XmlNode>;

XmlNodePtr parseXml(const std::vector<char>& data);
