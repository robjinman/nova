#include "vulkan/vulkan_window_delegate.hpp"
#include "vulkan/vulkan_utils.hpp"
#include <vulkan/vulkan_android.h>
#include <android/native_window.h>

namespace
{

class AndroidWindowDelegateImpl : public VulkanWindowDelegate
{
  public:
    AndroidWindowDelegateImpl(ANativeWindow& window);

    const std::vector<const char*>& getRequiredExtensions() const;
    VkSurfaceKHR createSurface(VkInstance instance);
    void getFrameBufferSize(int& width, int& height) const;

  private:
    ANativeWindow& m_window;
    std::vector<const char*> m_extensions;
};

AndroidWindowDelegateImpl::AndroidWindowDelegateImpl(ANativeWindow& window)
  : m_window(window)
{
  m_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  m_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
}

const std::vector<const char*>& AndroidWindowDelegateImpl::getRequiredExtensions() const
{
  return m_extensions;
}

VkSurfaceKHR AndroidWindowDelegateImpl::createSurface(VkInstance instance)
{
	VkSurfaceKHR surface{};

	VkAndroidSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
	info.window = &m_window;

	VK_CHECK(vkCreateAndroidSurfaceKHR(instance, &info, nullptr, &surface),
    "Failed to create surface");

	return surface;
}

void AndroidWindowDelegateImpl::getFrameBufferSize(int& width, int& height) const
{
  width = ANativeWindow_getWidth(&m_window);
  height = ANativeWindow_getHeight(&m_window);
}

}

WindowDelegatePtr createWindowDelegate(ANativeWindow& window)
{
  return std::make_unique<AndroidWindowDelegateImpl>(window);
}
