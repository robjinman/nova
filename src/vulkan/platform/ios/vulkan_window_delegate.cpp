#pragma once

#include "vulkan/vulkan_window_delegate.hpp"
#include "vulkan/vulkan_utils.hpp"

namespace
{

class IosWindowDelegateImpl : public VulkanWindowDelegate
{
  public:
    IosWindowDelegateImpl();

    const std::vector<const char*>& getRequiredExtensions() const;
    VkSurfaceKHR createSurface(VkInstance instance);
    void getFrameBufferSize(int& width, int& height) const;

  private:
    std::vector<const char*> m_extensions;
};

IosWindowDelegateImpl::IosWindowDelegateImpl()
{
  // TODO
}

const std::vector<const char*>& IosWindowDelegateImpl::getRequiredExtensions() const
{
  // TODO
}

VkSurfaceKHR IosWindowDelegateImpl::createSurface(VkInstance instance)
{
  // TODO
}

void IosWindowDelegateImpl::getFrameBufferSize(int& width, int& height) const
{
  // TODO
}

}

WindowDelegatePtr createWindowDelegate()
{
  return std::make_unique<IosWindowDelegateImpl>();
}
