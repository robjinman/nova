#pragma once

#include "exception.hpp"
#include <vulkan/vulkan.h>

#define VK_CHECK(fnCall, msg) \
  { \
    VkResult MAC_code = fnCall; \
    if (MAC_code != VK_SUCCESS) { \
      EXCEPTION(msg << " (result: " << MAC_code << ")"); \
    } \
  }
