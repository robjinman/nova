#pragma once

#include <vector>
#include <filesystem>
#include <memory>

class Directory
{
  public:
    class IteratorImpl
    {
      public:
        virtual std::filesystem::path get() const = 0;
        virtual void next() = 0;
        virtual bool operator==(const IteratorImpl& rhs) const = 0;

        virtual ~IteratorImpl() {}
    };

    class Iterator
    {
      public:
        Iterator(std::unique_ptr<IteratorImpl> impl)
          : m_impl(std::move(impl)) {}

        std::filesystem::path operator*() { return m_impl->get(); }
        bool operator==(const Iterator& rhs) const { return *m_impl == *rhs.m_impl; }
        bool operator!=(const Iterator& rhs) const { return !(*m_impl == *rhs.m_impl); }
        Iterator& operator++() { m_impl->next(); return *this; }

      private:
        std::unique_ptr<IteratorImpl> m_impl;
    };

    using IteratorPtr = std::unique_ptr<Iterator>;

    virtual Iterator begin() const = 0;
    virtual Iterator end() const = 0;

    virtual ~Directory() {}
};

using DirectoryPtr = std::unique_ptr<Directory>;

class FileSystem
{
  public:
    virtual std::vector<char> readFile(const std::filesystem::path& path) const = 0;
    virtual DirectoryPtr directory(const std::filesystem::path& path) const = 0;

    virtual ~FileSystem() {}
};

using FileSystemPtr = std::unique_ptr<FileSystem>;
