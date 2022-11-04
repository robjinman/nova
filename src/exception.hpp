#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

class Exception : public std::runtime_error {
public:
  Exception(const std::string& msg, const std::string& file, int line)
    : runtime_error(msg + " (file: " + file + ", line: " + std::to_string(line) + ")") {}
};

#define EXCEPTION(msg) \
  { \
    std::stringstream ss; \
    ss << msg; \
    throw Exception(ss.str(), __FILE__, __LINE__); \
  }

#define VK_CHECK(fnCall, msg) \
  { \
    VkResult code = fnCall; \
    if (code != VK_SUCCESS) { \
      EXCEPTION(msg << " (result: " << code << ")"); \
    } \
  }
