#pragma once

#include "exception.hpp"
#include <map>
#include <memory>
#include <list>
#include <cassert>

namespace tree_set
{

template<typename TKey, typename TData>
class Node
{
  public:
    using KeyPart = TKey;
    using Key = std::list<KeyPart>;

  private:
    using NodeMap = std::map<KeyPart, std::unique_ptr<Node<TKey, TData>>>;
    using NodeMapIterator = NodeMap::const_iterator;
    using Path = std::list<std::pair<const NodeMap*, NodeMapIterator>>;

  public:
    class Iterator
    {
      public:
        Iterator(Path path)
          : m_path(path)
        {
          firstChild(m_path);
        }

        bool operator==(const Iterator& rhs) const
        {
          return m_path == rhs.m_path;
        }

        const TData& operator*() const
        {
          assert(!m_path.empty());
          assert(m_path.back().second->second->isLeaf());

          return m_path.back().second->second->data();
        }

        const TData* operator->() const
        {
          return &(**this);
        }

        void next()
        {
          assert(m_path.size() > 0);

          m_path.back().second++;
          while (m_path.back().second == m_path.back().first->end()) {
            m_path.pop_back();
            if (m_path.empty()) {
              return;
            }
            m_path.back().second++;
          }

          firstChild(m_path);
        }

        Iterator& operator++()
        {
          next();
          return *this;
        }

      private:
        Path m_path;

        static void firstChild(Path& path)
        {
          if (!path.empty()) {
            auto& node = *path.back().second->second;
            if (!node.m_children.empty()) {
              path.push_back(std::pair{&node.m_children, node.m_children.cbegin()});
              firstChild(path);
            }
          }
        }
    };

    void insert(Key key, TData data)
    {
      if (key.empty()) {
        m_data = std::move(data);
        return;
      }

      KeyPart first = key.front();
      key.pop_front();

      auto i = m_children.find(first);
      if (i != m_children.end()) {
        i->second->insert(key, std::move(data));
      }
      else {
        auto child = std::make_unique<Node<TKey, TData>>();
        child->insert(key, std::move(data));
        m_children.insert(std::pair{first, std::move(child)}); 
      }
    }

    void clear()
    {
      m_children.clear();
      m_data = TData{};
    }

    const TData& data() const
    {
      return m_data;
    }

    Iterator begin() const
    {
      return Iterator(Path{std::pair{&m_children, m_children.cbegin()}});
    }

    Iterator end() const
    {
      return Iterator({});
    }

    Iterator find(const Key& k) const
    {
      ASSERT(!k.empty(), "Key is empty");

      Key key = k;
      Path path{};
      return find(path, key);
    }

  private:
    NodeMap m_children;
    TData m_data;

    Iterator find(Path& path, Key& key) const
    {
      if (key.empty()) {
        return isLeaf() ? Iterator{path} : end();
      }

      auto first = key.front();
      key.pop_front();

      auto i = m_children.find(first);
      if (i == m_children.end()) {
        return end();
      }

      path.push_back(std::pair{&m_children, i});

      return i->second->find(path, key);
    }

    bool isLeaf() const
    {
      return m_children.empty();
    }
};

} // namespace tree_set

template<typename TKey, typename TData>
using TreeSet = tree_set::Node<TKey, TData>;
