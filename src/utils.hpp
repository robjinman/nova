#pragma once

#include <sstream>

#define STR(x) [&]() {\
  std::stringstream MAC_ss; \
  MAC_ss << x; \
  return MAC_ss.str(); \
}()
