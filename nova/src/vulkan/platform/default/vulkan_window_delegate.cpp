#include "vulkan/vulkan_window_delegate.hpp"
#include "vulkan/vulkan_utils.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace
{

class VulkanWindowDelegateImpl : public VulkanWindowDelegate
{
  public:
    VulkanWindowDelegateImpl(GLFWwindow& window);

    const std::vector<const char*>& getRequiredExtensions() const;
    VkSurfaceKHR createSurface(VkInstance instance);
    void getFrameBufferSize(int& width, int& height) const;

  private:
    GLFWwindow& m_window;
    std::vector<const char*> m_extensions;
};

VulkanWindowDelegateImpl::VulkanWindowDelegateImpl(GLFWwindow& window)
  : m_window(window)
{
  uint32_t n = 0;
  const char** extensions = glfwGetRequiredInstanceExtensions(&n);
  for (uint32_t i = 0; i < n; ++i) {
    m_extensions.push_back(extensions[i]);
  }
}

const std::vector<const char*>& VulkanWindowDelegateImpl::getRequiredExtensions() const
{
  return m_extensions;
}

VkSurfaceKHR VulkanWindowDelegateImpl::createSurface(VkInstance instance)
{
  VkSurfaceKHR surface;

  VK_CHECK(glfwCreateWindowSurface(instance, &m_window, nullptr, &surface),
    "Failed to create window surface");

  return surface;
}

void VulkanWindowDelegateImpl::getFrameBufferSize(int& width, int& height) const
{
  glfwGetFramebufferSize(&m_window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(&m_window, &width, &height);
    glfwWaitEvents();
  }
}

}

WindowDelegatePtr createWindowDelegate(GLFWwindow& window)
{
  return std::make_unique<VulkanWindowDelegateImpl>(window);
}
