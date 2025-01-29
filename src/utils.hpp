#pragma once

#include <sstream>

#define STR(x) [&]() {\
  std::stringstream MAC_ss; \
  MAC_ss << x; \
  return MAC_ss.str(); \
}()

template<typename T>
bool inRange(T value, T min, T max)
{
  return value >= min && value <= max;
}

std::string versionString();
