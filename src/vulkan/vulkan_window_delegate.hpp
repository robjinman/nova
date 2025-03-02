#pragma once

#include "window_delegate.hpp"
#include <vulkan/vulkan.h>
#include <vector>

class VulkanWindowDelegate : public WindowDelegate
{
  public:
    virtual const std::vector<const char*>& getRequiredExtensions() const = 0;
    virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;
    virtual void getFrameBufferSize(int& width, int& height) const = 0;

    virtual ~VulkanWindowDelegate() {}
};
