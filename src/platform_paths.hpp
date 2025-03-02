#pragma once

#include <filesystem>
#include <memory>
#include <map>

using DirectoryMap = std::map<std::string, std::filesystem::path>;

class PlatformPaths
{
  public:
    PlatformPaths(const DirectoryMap& directories);

    std::filesystem::path get(const std::string& directory) const;
    std::filesystem::path get(const std::string& directory, const std::string& name) const;

  private:
    DirectoryMap m_directories;
};

using PlatformPathsPtr = std::unique_ptr<PlatformPaths>;
