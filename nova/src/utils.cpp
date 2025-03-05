#include "utils.hpp"
#include "version.hpp"
#include "exception.hpp"
#include <fstream>

std::string versionString()
{
  return STR("Nova " << Nova_VERSION_MAJOR << "." << Nova_VERSION_MINOR);
}

std::vector<char> readBinaryFile(const std::string& filename)
{
  std::ifstream stream(filename, std::ios::ate | std::ios::binary);

  if (!stream.is_open()) {
    EXCEPTION("Failed to open file " << filename);
  }

  size_t fileSize = stream.tellg();
  std::vector<char> bytes(fileSize);

  stream.seekg(0);
  stream.read(bytes.data(), fileSize);

  return bytes;
}
