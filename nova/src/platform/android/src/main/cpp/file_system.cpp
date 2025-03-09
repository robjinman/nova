#include "file_system.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include <android/asset_manager.h>

namespace
{

class AndroidIteratorImpl : public Directory::IteratorImpl
{
  public:
    AndroidIteratorImpl(AAssetDir* assetDir);

    std::filesystem::path get() const override;
    void next() override;
    bool operator==(const IteratorImpl& rhs) const override;

    ~AndroidIteratorImpl() override;

  private:
    AAssetDir* m_assetDir;
    std::filesystem::path m_path;
};

AndroidIteratorImpl::AndroidIteratorImpl(AAssetDir* assetDir)
  : m_assetDir(assetDir)
{
  if (assetDir != nullptr) {
    next();
  }
}

std::filesystem::path AndroidIteratorImpl::get() const
{
  return m_path;
}

void AndroidIteratorImpl::next()
{
  ASSERT(m_assetDir != nullptr, "Advanced beyond end iterator");

  const char* path = AAssetDir_getNextFileName(m_assetDir);
  if (path != nullptr) {
    m_path = path;
  }
  else if (m_assetDir != nullptr) {
    AAssetDir_close(m_assetDir);
    m_assetDir = nullptr;
    m_path = std::filesystem::path{};
  }
}

bool AndroidIteratorImpl::operator==(const IteratorImpl& rhs) const
{
  return m_path == dynamic_cast<const AndroidIteratorImpl&>(rhs).m_path;
}

AndroidIteratorImpl::~AndroidIteratorImpl()
{
  if (m_assetDir != nullptr) {
    AAssetDir_close(m_assetDir);
  }
}

class AndroidDirectoryImpl : public Directory
{
  public:
    AndroidDirectoryImpl(AAssetManager& assetManager, const std::filesystem::path& path);

    Directory::Iterator begin() const override;
    Directory::Iterator end() const override;

  private:
    AAssetManager& m_assetManager;
    std::filesystem::path m_path;
};

AndroidDirectoryImpl::AndroidDirectoryImpl(AAssetManager& assetManager,
  const std::filesystem::path& path)
  : m_assetManager(assetManager)
  , m_path(path)
{
}

Directory::Iterator AndroidDirectoryImpl::begin() const
{
  auto assetDir = AAssetManager_openDir(&m_assetManager, m_path.c_str());
  return Directory::Iterator{std::make_unique<AndroidIteratorImpl>(assetDir)};
}

Directory::Iterator AndroidDirectoryImpl::end() const
{
  return Directory::Iterator{std::make_unique<AndroidIteratorImpl>(nullptr)};
}

class AndroidFileSystem : public FileSystem
{
  public:
    AndroidFileSystem(AAssetManager& assetManager);

    std::vector<char> readFile(const std::filesystem::path& path) const override;
    DirectoryPtr directory(const std::filesystem::path& path) const override;

  private:
    AAssetManager& m_assetManager;
};

AndroidFileSystem::AndroidFileSystem(AAssetManager& assetManager)
  : m_assetManager(assetManager)
{
}

std::vector<char> AndroidFileSystem::readFile(const std::filesystem::path& path) const
{
  AAsset* asset = AAssetManager_open(&m_assetManager, path.c_str(), AASSET_MODE_BUFFER);
  size_t assetLength = AAsset_getLength(asset);
  std::vector<char> data(assetLength);

  size_t read = 0;
  int result = 0;
  do {
    result = AAsset_read(asset, data.data() + read, assetLength - read);
    read += result;
  } while (result > 0);

  return data;
}

DirectoryPtr AndroidFileSystem::directory(const std::filesystem::path& path) const
{
  return std::make_unique<AndroidDirectoryImpl>(m_assetManager, path);
}

} // namespace

FileSystemPtr createAndroidFileSystem(AAssetManager& assetManager)
{
  return std::make_unique<AndroidFileSystem>(assetManager);
}
