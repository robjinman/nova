#include "vulkan/vulkan_window_delegate.hpp"
#include "vulkan/vulkan_utils.hpp"
#include <vulkan/vulkan_metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace
{

class IosWindowDelegateImpl : public VulkanWindowDelegate
{
  public:
    IosWindowDelegateImpl(CAMetalLayer* metalLayer);

    const std::vector<const char*>& getRequiredExtensions() const;
    VkSurfaceKHR createSurface(VkInstance instance);
    void getFrameBufferSize(int& width, int& height) const;

  private:
    CAMetalLayer* m_metalLayer;
    std::vector<const char*> m_extensions;
};

IosWindowDelegateImpl::IosWindowDelegateImpl(CAMetalLayer* metalLayer)
  : m_metalLayer(metalLayer)
{
  m_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  m_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  m_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  m_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  m_extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
}

const std::vector<const char*>& IosWindowDelegateImpl::getRequiredExtensions() const
{
  return m_extensions;
}

VkSurfaceKHR IosWindowDelegateImpl::createSurface(VkInstance instance)
{
	VkSurfaceKHR surface{};

	VkMetalSurfaceCreateInfoEXT info{VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
	info.pLayer = m_metalLayer;

	VK_CHECK(vkCreateMetalSurfaceEXT(instance, &info, nullptr, &surface), "Failed to create surface");

	return surface;
}

void IosWindowDelegateImpl::getFrameBufferSize(int& width, int& height) const
{
  CGSize size = m_metalLayer.drawableSize;
  width = static_cast<int>(size.width);
  height = static_cast<int>(size.height);
}

}

WindowDelegatePtr createWindowDelegate(CAMetalLayer* metalLayer)
{
  return std::make_unique<IosWindowDelegateImpl>(metalLayer);
}
