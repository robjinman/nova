#include "utils.hpp"
#include "version.hpp"

std::string versionString()
{
  return STR("Nova " << Nova_VERSION_MAJOR << "." << Nova_VERSION_MINOR);
}
