#include "file_system.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include <fstream>

namespace
{

class DefaultIteratorImpl : public Directory::IteratorImpl
{
  public:
    DefaultIteratorImpl(std::filesystem::directory_iterator iterator);

    std::filesystem::path get() const override;
    void next() override;
    bool operator==(const IteratorImpl& rhs) const override;

  private:
    std::filesystem::directory_iterator m_iterator;
};

DefaultIteratorImpl::DefaultIteratorImpl(std::filesystem::directory_iterator iterator)
  : m_iterator(iterator)
{
}

std::filesystem::path DefaultIteratorImpl::get() const
{
  return *m_iterator;
}

void DefaultIteratorImpl::next()
{
  ++m_iterator;
}

bool DefaultIteratorImpl::operator==(const IteratorImpl& rhs) const
{
  return m_iterator == dynamic_cast<const DefaultIteratorImpl&>(rhs).m_iterator;
}

class DefaultDirectoryImpl : public Directory
{
  public:
    DefaultDirectoryImpl(const std::filesystem::path& path);

    Directory::Iterator begin() const override;
    Directory::Iterator end() const override;

  private:
    std::filesystem::path m_path;
};

DefaultDirectoryImpl::DefaultDirectoryImpl(const std::filesystem::path& path)
  : m_path(path)
{
}

Directory::Iterator DefaultDirectoryImpl::begin() const
{
  auto i = std::filesystem::directory_iterator{m_path};
  return Directory::Iterator{std::make_unique<DefaultIteratorImpl>(i)};
}

Directory::Iterator DefaultDirectoryImpl::end() const
{
  auto i = std::filesystem::end(std::filesystem::directory_iterator{m_path});
  return Directory::Iterator{std::make_unique<DefaultIteratorImpl>(i)};
}

class DefaultFileSystem : public FileSystem
{
  public:
    DefaultFileSystem(const std::filesystem::path& dataRootDir);

    std::vector<char> readFile(const std::filesystem::path& path) const override;
    DirectoryPtr directory(const std::filesystem::path& path) const override;

  private:
    std::filesystem::path m_dataRootDir;
};

DefaultFileSystem::DefaultFileSystem(const std::filesystem::path& dataRootDir)
  : m_dataRootDir(dataRootDir)
{
}

std::vector<char> DefaultFileSystem::readFile(const std::filesystem::path& path) const
{
  return readBinaryFile(m_dataRootDir / path);
}

DirectoryPtr DefaultFileSystem::directory(const std::filesystem::path& path) const
{
  return std::make_unique<DefaultDirectoryImpl>(m_dataRootDir / path);
}

} // namespace

FileSystemPtr createDefaultFileSystem(const std::filesystem::path& dataRootDir)
{
  return std::make_unique<DefaultFileSystem>(dataRootDir);
}
