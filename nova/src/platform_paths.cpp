#include "platform_paths.hpp"
#include "exception.hpp"

namespace fs = std::filesystem;

namespace
{

const fs::path& assertExists(const fs::path& path)
{
  ASSERT(fs::exists(path), "Path " << path << " does not exist");
  return path;
}

}

PlatformPaths::PlatformPaths(const DirectoryMap& directories)
  : m_directories(directories)
{}

fs::path PlatformPaths::get(const std::string& directory) const
{
  auto i = m_directories.find(directory);
  ASSERT(i != m_directories.end(), "Unrecognised application directory: " << directory);

  return assertExists(i->second);
}

fs::path PlatformPaths::get(const std::string& directory, const std::string& name) const
{
  auto dir = get(directory);
  return assertExists(dir.append(name));
}
