#pragma once

#include <sstream>
#include <vector>
#include <concepts>

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

template<std::floating_point T>
T parseFloat(const std::string& s)
{
  return static_cast<T>(std::stod(s));
}

std::vector<char> readBinaryFile(const std::string& filename);

std::string versionString();
