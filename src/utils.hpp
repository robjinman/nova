#pragma once

#include <sstream>

#define STR(x) [&]() {\
  std::stringstream ss; \
  ss << x; \
  return ss.str(); \
}()
