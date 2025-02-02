#pragma once

#include "exception.hpp"
#include <ostream>
#include <map>
#include <memory>
#include <list>
#include <cassert>

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
          first(m_path);
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

          first(m_path);
        }

        Iterator& operator++()
        {
          next();
          return *this;
        }

      private:
        Path m_path;

        static void first(Path& path)
        {
          if (!path.empty()) {
            auto& node = *path.back().second->second;
            if (!node.m_children.empty()) {
              path.push_back(std::pair{&node.m_children, node.m_children.cbegin()});
              first(path);
            }
          }
        }
    };

    void add(Key key, TData data)
    {
      if (key.empty()) {
        m_data = std::move(data);
        return;
      }

      KeyPart first = key.front();
      key.pop_front();

      auto i = m_children.find(first);
      if (i != m_children.end()) {
        i->second->add(key, std::move(data));
      }
      else {
        auto child = std::make_unique<Node<TKey, TData>>();
        child->add(key, std::move(data));
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

    TData& data()
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

    Iterator find(const Key& k)
    {
      ASSERT(!k.empty(), "Key is empty");

      Key key = k;
      Path path{};
      return find(path, key);
    }

    void dbg_print(std::ostream& stream, size_t depth = 0) const
    {
      if (isLeaf()) {
        stream << std::string(depth, '\t') << m_data << "\n";
      }
      else {
        for (auto& i : m_children) {
          stream << std::string(depth, '\t') << i.first << ":\n";
          i.second->dbg_print(stream, depth + 1);
        }
      }
    }

  private:
    NodeMap m_children;
    TData m_data;

    Iterator find(Path& path, Key& key)
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

template<typename TKey, typename TData>
using TreeSet = Node<TKey, TData>;
