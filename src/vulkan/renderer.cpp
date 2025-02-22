#include "vulkan/vulkan_utils.hpp"
#include "vulkan/default_pipeline.hpp"
#include "vulkan/instanced_pipeline.hpp"
#include "vulkan/skybox_pipeline.hpp"
#include "vulkan/render_resources.hpp"
#include "renderer.hpp"
#include "exception.hpp"
#include "version.hpp"
#include "logger.hpp"
#include "camera.hpp"
#include "utils.hpp"
#include "time.hpp"
#include "thread.hpp"
#include "triple_buffer.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <optional>
#include <fstream>
#include <set>
#include <map>
#include <cassert>

namespace
{

const float_t DRAW_DISTANCE = 10000.f;

const std::vector<const char*> ValidationLayers = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DeviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
  "VK_KHR_portability_subset",
#endif
};

enum class PipelineName : RenderGraphKey
{
  defaultModel,
  instancedModel,
  skybox
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() const
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class RendererImpl : public Renderer
{
  public:
    RendererImpl(GLFWwindow& window, Logger& logger);

    void start() override;
    double frameRate() const override;

    // Resources
    //
    RenderItemId addTexture(TexturePtr texture) override;
    RenderItemId addCubeMap(std::array<TexturePtr, 6>&& textures) override;
    void removeTexture(RenderItemId id) override;
    void removeCubeMap(RenderItemId id) override;

    // Meshes
    //
    RenderItemId addMesh(MeshPtr mesh) override;
    void removeMesh(RenderItemId id) override;

    // Materials
    //
    RenderItemId addMaterial(MaterialPtr material) override;
    void removeMaterial(RenderItemId id) override;

    // Per frame draw functions
    //
    void beginFrame(const Camera& camera) override;
    void stageInstance(RenderItemId mesh, RenderItemId material, const Mat4x4f& transform) override;
    void stageModel(RenderItemId mesh, RenderItemId material, const Mat4x4f& transform) override;
    void stageSkybox(RenderItemId mesh, RenderItemId material) override;
    void endFrame() override;

    ~RendererImpl() override;

  private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
      const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData);

    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    bool isPhysicalDeviceSuitable(VkPhysicalDevice device) const;
    void checkValidationLayerSupport() const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
    std::vector<const char*> getRequiredExtensions() const;
    void setupDebugMessenger();
    void destroyDebugMessenger();
    VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo() const;
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    void createLogicalDevice();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& formats) const;
    VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& modes) const;
    VkExtent2D chooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    void createSwapChain();
    void recreateSwapChain();
    void cleanupSwapChain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createPipelines();
    void createDepthResources();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void updateResources();
    void updateUniformBuffer(const Mat4x4f& cameraMatrix);
    void finishFrame();
    void createSyncObjects();
    VkFormat findDepthFormat() const;
    void renderLoop();
    void cleanUp();

    GLFWwindow& m_window;
    Logger& m_logger;
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceLimits m_deviceLimits;
    VkSurfaceKHR m_surface;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkSwapchainKHR m_swapchain;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;
    std::vector<VkCommandBuffer> m_commandBuffers;
    uint32_t m_imageIndex;
    VkRenderPass m_renderPass;
    VkCommandPool m_commandPool;

    size_t m_currentFrame = 0;
    bool m_framebufferResized = false;  // TODO
    Mat4x4f m_projectionMatrix;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    struct RenderState
    {
      RenderGraph graph;
      Mat4x4f cameraMatrix;
    };

    TripleBuffer<RenderState> m_renderStates;
  
    RenderResourcesPtr m_resources;
    std::map<PipelineName, PipelinePtr> m_pipelines;

    Timer m_timer;
    std::atomic<double> m_frameRate;

    Thread m_thread;
    std::atomic<bool> m_running;
};

RendererImpl::RendererImpl(GLFWwindow& window, Logger& logger)
  : m_window(window)
  , m_logger(logger)
{
  m_thread.run<void>([this]() {
    createInstance();
#ifndef NDEBUG
    setupDebugMessenger();
#endif
  }).wait();
  createSurface();
  m_thread.run<void>([this]() {
    pickPhysicalDevice();
    createLogicalDevice();
  }).wait();
  createSwapChain();
  m_thread.run<void>([this]() {
    createImageViews();
    createRenderPass();
    createCommandPool();
    m_resources = createRenderResources(m_physicalDevice, m_device, m_graphicsQueue, m_commandPool);
    createPipelines();
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
    createSyncObjects();
  }).wait();

  float_t aspectRatio = m_swapchainExtent.width / static_cast<float_t>(m_swapchainExtent.height);
  m_projectionMatrix = perspective(degreesToRadians(45.f), aspectRatio, 0.1f, DRAW_DISTANCE);
}

void RendererImpl::start()
{
  m_running = true;
  m_thread.run<void>([&]() {
    renderLoop();
  });
}

double RendererImpl::frameRate() const
{
  return m_frameRate;
}

void RendererImpl::stageInstance(RenderItemId meshId, RenderItemId materialId,
  const Mat4x4f& transform)
{
  RenderGraph::Key key{
    static_cast<RenderGraphKey>(PipelineName::instancedModel),
    static_cast<RenderGraphKey>(meshId),
    static_cast<RenderGraphKey>(materialId)
  };
  InstancedModelNode* node = nullptr;
  auto i = m_renderStates.getWritable().graph.find(key);
  if (i != m_renderStates.getWritable().graph.end()) {
    node = dynamic_cast<InstancedModelNode*>(i->get());
  }
  else {
    auto newNode = std::make_unique<InstancedModelNode>();
    newNode->mesh = meshId;
    newNode->material = materialId;
    node = newNode.get();
    m_renderStates.getWritable().graph.insert(key, std::move(newNode));
  }
  node->instances.push_back(MeshInstance{transform});
}

void RendererImpl::stageModel(RenderItemId mesh, RenderItemId material, const Mat4x4f& transform)
{
  static RenderGraphKey nextId = 0; // TODO

  auto node = std::make_unique<DefaultModelNode>();
  node->mesh = mesh;
  node->material = material;
  node->modelMatrix = transform;

  RenderGraph::Key key{
    static_cast<RenderGraphKey>(PipelineName::defaultModel),
    static_cast<RenderGraphKey>(mesh),
    static_cast<RenderGraphKey>(material),
    nextId++
  };
  m_renderStates.getWritable().graph.insert(key, std::move(node));
}

void RendererImpl::stageSkybox(RenderItemId mesh, RenderItemId material)
{
  auto node = std::make_unique<SkyboxNode>();
  node->mesh = mesh;
  node->material = material;

  RenderGraph::Key key{ static_cast<RenderGraphKey>(PipelineName::skybox) };
  m_renderStates.getWritable().graph.insert(key, std::move(node));
}

void RendererImpl::beginFrame(const Camera& camera)
{
  auto& state = m_renderStates.getWritable();
  state.graph.clear();
  state.cameraMatrix = camera.getMatrix();
}

void RendererImpl::renderLoop()
{
  while (m_running) {
    VK_CHECK(vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX),
      "Error waiting for fence");

    VkResult acqImgResult = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
      m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex);

    if (acqImgResult == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain();
      return;
    }
    else if (acqImgResult != VK_SUCCESS && acqImgResult != VK_SUBOPTIMAL_KHR) {
      EXCEPTION("Error obtaining image from swap chain");
    }

    VK_CHECK(vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]),
      "Error resetting fence");

    vkResetCommandBuffer(m_commandBuffers[m_imageIndex], 0);

    updateUniformBuffer(m_renderStates.getReadable().cameraMatrix);

    updateResources();
    recordCommandBuffer(m_commandBuffers[m_imageIndex], m_imageIndex);
    finishFrame();

    m_renderStates.readComplete();

    m_frameRate = 1.0 / m_timer.elapsed();
    m_timer.reset();
  }

  cleanUp();
}

void RendererImpl::updateResources()
{
  auto& renderGraph = m_renderStates.getReadable().graph;

  for (auto& node : renderGraph) {
    switch (node->type) {
      case RenderNodeType::instancedModel: {
        auto& nodeData = dynamic_cast<const InstancedModelNode&>(*node);
        m_resources->updateMeshInstances(nodeData.mesh, nodeData.instances);

        break;
      }
      default: break;
    }
  }
}

void RendererImpl::endFrame()
{
  m_renderStates.writeComplete();
}

void RendererImpl::finishFrame()
{
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_commandBuffers[m_imageIndex];

  VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]),
    "Failed to submit draw command buffer");

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  VkSwapchainKHR swapchains[] = { m_swapchain };
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &m_imageIndex;
  presentInfo.pResults = nullptr;

  VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);
  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR
    || m_framebufferResized) {

    m_framebufferResized = false;
    recreateSwapChain();
  }
  else if (presentResult != VK_SUCCESS) {
    EXCEPTION("Failed to present swap chain image");
  }

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererImpl::updateUniformBuffer(const Mat4x4f& cameraMatrix)
{
  UniformBufferObject ubo{};
  ubo.viewMatrix = cameraMatrix;
  ubo.projMatrix = m_projectionMatrix;

  m_resources->updateUniformBuffer(ubo, m_currentFrame);
}

void RendererImpl::removeMesh(RenderItemId id)
{
  // TODO
  EXCEPTION("Not implemented");
  //m_resources->removeMesh(id);
}

void RendererImpl::removeTexture(RenderItemId id)
{
  // TODO
  EXCEPTION("Not implemented");
  //m_resources->removeTexture(id);
}

void RendererImpl::removeCubeMap(RenderItemId id)
{
  // TODO
  EXCEPTION("Not implemented");
  //m_resources->removeCubeMap(id);
}

void RendererImpl::removeMaterial(RenderItemId id)
{
  // TODO
  EXCEPTION("Not implemented");
  //m_resources->removeMaterial(id);
}

VkExtent2D RendererImpl::chooseSwapChainExtent(
  const VkSurfaceCapabilitiesKHR& capabilities) const
{
  if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(&m_window, &width, &height);

    VkExtent2D extent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
    };

    extent.width = std::max(capabilities.minImageExtent.width,
      std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(width)));
    extent.height = std::max(capabilities.minImageExtent.height,
      std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(height)));

    return extent;
  }
  else {
    return capabilities.currentExtent;
  }
}

VkPresentModeKHR RendererImpl::chooseSwapChainPresentMode(
  const std::vector<VkPresentModeKHR>& availableModes) const
{
  for (auto& mode : availableModes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR RendererImpl::chooseSwapChainSurfaceFormat(
  const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
  for (const auto& format : availableFormats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }

  assert(availableFormats.size() > 0);

  return availableFormats[0];
}

void RendererImpl::createSwapChain()
{
  auto swapchainSupport = querySwapChainSupport(m_physicalDevice);
  auto surfaceFormat = chooseSwapChainSurfaceFormat(swapchainSupport.formats);
  auto presentMode = chooseSwapChainPresentMode(swapchainSupport.presentModes);
  auto extent = chooseSwapChainExtent(swapchainSupport.capabilities);

  uint32_t minImageCount = swapchainSupport.capabilities.minImageCount + 1;
  if (swapchainSupport.capabilities.maxImageCount != 0) {
    minImageCount = std::min(minImageCount, swapchainSupport.capabilities.maxImageCount);
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = m_surface;
  createInfo.minImageCount = minImageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  auto indices = findQueueFamilies(m_physicalDevice);
  uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
  if (indices.graphicsFamily == indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  else {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }

  createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain),
    "Failed to create swap chain");

  uint32_t imageCount;
  VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr),
    "Failed to retrieve swap chain images");

  m_swapchainImages.resize(imageCount);
  VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data()),
    "Failed to retrieve swap chain images");

  m_swapchainImageFormat = surfaceFormat.format;
  m_swapchainExtent = extent;
}

void RendererImpl::cleanupSwapChain()
{
  vkDestroyImageView(m_device, m_depthImageView, nullptr);
  vkDestroyImage(m_device, m_depthImage, nullptr);
  vkFreeMemory(m_device, m_depthImageMemory, nullptr);

  for (auto framebuffer : m_swapchainFramebuffers) {
    vkDestroyFramebuffer(m_device, framebuffer, nullptr);
  }
  for (auto imageView : m_swapchainImageViews) {
    vkDestroyImageView(m_device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void RendererImpl::recreateSwapChain()
{
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(&m_window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(&m_window, &width, &height);
    glfwWaitEvents();
  }

  VK_CHECK(vkDeviceWaitIdle(m_device), "Error waiting for device to be idle");

  cleanupSwapChain();
  createSwapChain();
  createImageViews();
  createDepthResources();
  createFramebuffers();

  // TODO: Might need to recreate renderpass if the swap chain image format has changed if, for
  // example, the window has moved from a standard to a high dynamic range monitor.
}

SwapChainSupportDetails RendererImpl::querySwapChainSupport(VkPhysicalDevice device) const
{
  SwapChainSupportDetails details;

  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities),
    "Failed to retrieve physical device surface capabilities");

  uint32_t formatCount;
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr),
    "Failed to retrieve physical device surface formats");

  details.formats.resize(formatCount);
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount,
    details.formats.data()), "Failed to retrieve physical device surface formats");

  uint32_t presentModeCount;
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr),
    "Failed to retrieve physical device surface present modes");

  details.presentModes.resize(presentModeCount);
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount,
    details.presentModes.data()), "Failed to retrieve physical device surface present modes");

  return details;
}

QueueFamilyIndices RendererImpl::findQueueFamilies(VkPhysicalDevice device) const
{
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport),
      "Failed to check present support for device");

    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }
  }

  return indices;
}

VKAPI_ATTR VkBool32 VKAPI_CALL RendererImpl::debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
  const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
{
  std::cerr << "Validation layer: " << data->pMessage << std::endl;

  return VK_FALSE;
}

std::vector<const char*> RendererImpl::getRequiredExtensions() const
{
  std::vector<const char*> extensions;

  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
  extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
    extensions.push_back(glfwExtensions[i]);
  }

#ifndef NDEBUG
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  return extensions;
}

bool RendererImpl::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
  uint32_t count;
  VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr),
    "Failed to enumerate device extensions");

  std::vector<VkExtensionProperties> available(count);

  VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data()),
    "Failed to enumerate device extensions");

  for (auto extension : DeviceExtensions) {
    auto fnMatches = [=](const VkExtensionProperties& p) {
      return strcmp(extension, p.extensionName) == 0;
    };
    if (std::find_if(available.begin(), available.end(), fnMatches) == available.end()) {
      return false;
    }
  }
  return true;
}

void RendererImpl::checkValidationLayerSupport() const
{
  uint32_t layerCount;
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr),
    "Failed to enumerate instance layer properties");

  std::vector<VkLayerProperties> available(layerCount);
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, available.data()),
    "Failed to enumerate instance layer properties");

  for (auto layer : ValidationLayers) {
    auto fnMatches = [=](const VkLayerProperties& p) {
      return strcmp(layer, p.layerName) == 0;
    };
    if (std::find_if(available.begin(), available.end(), fnMatches) == available.end()) {
      EXCEPTION("Validation layer '" << layer << "' not supported");
    }
  }
}

bool RendererImpl::isPhysicalDeviceSuitable(VkPhysicalDevice device) const
{
  bool extensionsSupported = checkDeviceExtensionSupport(device);

  if (!extensionsSupported) {
    m_logger.warn("Extensions not supported");
    return false;
  }

  auto swapchainSupport = querySwapChainSupport(device);
  bool swapchainAdequate = !swapchainSupport.formats.empty() &&
                           !swapchainSupport.presentModes.empty();

  auto indices = findQueueFamilies(device);

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return swapchainAdequate && indices.isComplete() && supportedFeatures.samplerAnisotropy;
}

void RendererImpl::createLogicalDevice()
{
  auto indices = findQueueFamilies(m_physicalDevice);
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
    indices.graphicsFamily.value(),
    indices.presentFamily.value()
  };

  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};

    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = queueCreateInfos.size();
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = DeviceExtensions.size();
  createInfo.ppEnabledExtensionNames = DeviceExtensions.data();

#ifdef NDEBUG
  createInfo.enabledLayerCount = 0;
#else
  createInfo.enabledLayerCount = ValidationLayers.size();
  createInfo.ppEnabledLayerNames = ValidationLayers.data();
#endif

  VK_CHECK(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device),
    "Failed to create logical device");

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

void RendererImpl::createImageViews()
{
  m_swapchainImageViews.resize(m_swapchainImages.size());

  for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
    m_swapchainImageViews[i] = createImageView(m_device, m_swapchainImages[i],
      m_swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);
  }
}

void RendererImpl::createRenderPass()
{
  VkAttachmentDescription colourAttachment{};
  colourAttachment.format = m_swapchainImageFormat;
  colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colourAttachmentRef{};
  colourAttachmentRef.attachment = 0;
  colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = findDepthFormat();
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colourAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  std::array<VkAttachmentDescription, 2> attachments = {
    colourAttachment,
    depthAttachment
  };

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = attachments.size();
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                           | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass),
    "Failed to create render pass");
}

void RendererImpl::createFramebuffers()
{
  m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

  for (size_t i = 0; i < m_swapchainImageViews.size(); ++i) {
    std::array<VkImageView, 2> attachments = { m_swapchainImageViews[i], m_depthImageView };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = attachments.size();
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = m_swapchainExtent.width;
    framebufferInfo.height = m_swapchainExtent.height;
    framebufferInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]),
      "Failed to create framebuffer");
  }
}

void RendererImpl::createCommandPool()
{
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool),
    "Failed to create command pool");
}

void RendererImpl::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo),
    "Failed to begin recording command buffer");

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass;
  renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = { 0, 0 };
  renderPassInfo.renderArea.extent = m_swapchainExtent;
  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
  clearValues[1].depthStencil = { 1.0f, 0 };
  renderPassInfo.clearValueCount = clearValues.size();
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  auto choosePipeline = [](RenderNodeType nodeType) {
    switch (nodeType) {
      case RenderNodeType::defaultModel: return PipelineName::defaultModel;
      case RenderNodeType::instancedModel: return PipelineName::instancedModel;
      case RenderNodeType::skybox: return PipelineName::skybox;
    }
    assert(false);
    return PipelineName::defaultModel;
  };

  const auto& renderGraph = m_renderStates.getReadable().graph;
  for (auto& node : renderGraph) {
    auto& pipeline = m_pipelines.at(choosePipeline(node->type));
    pipeline->recordCommandBuffer(commandBuffer, *node, m_currentFrame);
  }

  vkCmdEndRenderPass(commandBuffer);

  VK_CHECK(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer");
}

void RendererImpl::createCommandBuffers()
{
  m_commandBuffers.resize(m_swapchainFramebuffers.size());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = m_commandBuffers.size();

  VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()),
    "Failed to allocate command buffers");
}

RenderItemId RendererImpl::addTexture(TexturePtr texture)
{
  ASSERT(!m_running, "Renderer already started"); // TODO
  return m_thread.run<RenderItemId>([&, this]() {
    return m_resources->addTexture(std::move(texture));
  }).get();
}

RenderItemId RendererImpl::addCubeMap(std::array<TexturePtr, 6>&& textures)
{
  ASSERT(!m_running, "Renderer already started"); // TODO
  return m_thread.run<RenderItemId>([&, this]() {
    return m_resources->addCubeMap(std::move(textures));
  }).get();
}

RenderItemId RendererImpl::addMaterial(MaterialPtr material)
{
  ASSERT(!m_running, "Renderer already started"); // TODO
  return m_thread.run<RenderItemId>([&, this]() {
    return m_resources->addMaterial(std::move(material));
  }).get();
}

RenderItemId RendererImpl::addMesh(MeshPtr mesh)
{
  ASSERT(!m_running, "Renderer already started"); // TODO
  return m_thread.run<RenderItemId>([&, this]() {
    return m_resources->addMesh(std::move(mesh));
  }).get();
}

void RendererImpl::createSyncObjects()
{
  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]),
      "Failed to create semaphore");
    VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]),
      "Failed to create semaphore");
    VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]),
      "Failed to create fence");
  }
}

VkFormat RendererImpl::findDepthFormat() const
{
  auto findSupportedFormat = [this](const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features) {

    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
        return format;
      }
      else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
        (props.optimalTilingFeatures & features) == features) {

        return format;
      }
    }

    EXCEPTION("Failed to find supported format");
  };

  return findSupportedFormat({
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT
  }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void RendererImpl::createDepthResources()
{
  VkFormat depthFormat = findDepthFormat();

  createImage(m_physicalDevice, m_device, m_swapchainExtent.width, m_swapchainExtent.height, depthFormat,
    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);

  m_depthImageView = createImageView(m_device, m_depthImage, depthFormat,
    VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);
}

void RendererImpl::createPipelines()
{
  m_pipelines[PipelineName::defaultModel] = std::make_unique<DefaultPipeline>(m_device,
    m_swapchainExtent, m_renderPass, *m_resources);
  m_pipelines[PipelineName::instancedModel] = std::make_unique<InstancedPipeline>(m_device,
    m_swapchainExtent, m_renderPass, *m_resources);
  m_pipelines[PipelineName::skybox] = std::make_unique<SkyboxPipeline>(m_device,
    m_swapchainExtent, m_renderPass, *m_resources);
}

void RendererImpl::createSurface()
{
  VK_CHECK(glfwCreateWindowSurface(m_instance, &m_window, nullptr, &m_surface),
    "Failed to create window surface");
}

void RendererImpl::pickPhysicalDevice()
{
  uint32_t deviceCount = 0;
  VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr),
    "Failed to enumerate physical devices");

  if (deviceCount == 0) {
    EXCEPTION("No physical devices found");
  }

  DBG_LOG(m_logger, STR("Found " << deviceCount << " devices"));

  std::vector<VkPhysicalDevice> devices(deviceCount);
  VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data()),
    "Failed to enumerate physical devices");

  VkPhysicalDeviceProperties props;

  const std::map<int, size_t> deviceTypePriority{
    { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 0 },
    { VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, 1 },
    { VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, 2 },
    { VK_PHYSICAL_DEVICE_TYPE_CPU, 3 },
    { VK_PHYSICAL_DEVICE_TYPE_OTHER, 4}
  };

  // (priority, device index)
  std::set<std::pair<size_t, size_t>> sortedDevices;

  for (size_t i = 0; i < deviceCount; ++i) {
    vkGetPhysicalDeviceProperties(devices[i], &props);

    DBG_LOG(m_logger, STR("Device: " << props.deviceName));
    DBG_LOG(m_logger, STR("Type: " << props.deviceType));

    DBG_LOG(m_logger, "Physical device properties");
    DBG_LOG(m_logger, STR("  maxComputeWorkGroupSize: "
      << props.limits.maxComputeWorkGroupSize[0] << ", "
      << props.limits.maxComputeWorkGroupSize[1] << ", "
      << props.limits.maxComputeWorkGroupSize[2]));
    DBG_LOG(m_logger, STR("  maxComputeWorkGroupCount: "
      << props.limits.maxComputeWorkGroupCount[0] << ", "
      << props.limits.maxComputeWorkGroupCount[1] << ", "
      << props.limits.maxComputeWorkGroupCount[2]));
    DBG_LOG(m_logger, STR("  maxComputeWorkGroupInvocations: "
      << props.limits.maxComputeWorkGroupInvocations));

    size_t priority = deviceTypePriority.at(props.deviceType);
    sortedDevices.insert(std::make_pair(priority, i));
  }

  assert(!sortedDevices.empty());
  int index = -1;
  for (auto& dev : sortedDevices) {
    if (isPhysicalDeviceSuitable(devices[dev.second])) {
      index = dev.second;
      break;
    }
  }
  if (index == -1) {
    EXCEPTION("No suitable physical device found");
  }

  vkGetPhysicalDeviceProperties(devices[index], &props);

  DBG_LOG(m_logger, STR("Selecting " << props.deviceName));
  m_deviceLimits = props.limits;

  m_physicalDevice = devices[index];
}

void RendererImpl::createInstance()
{
#ifndef NDEBUG
  checkValidationLayerSupport();
#endif

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Project Nova";
  appInfo.applicationVersion = VK_MAKE_VERSION(Nova_VERSION_MAJOR, Nova_VERSION_MINOR, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
#ifdef __APPLE__
  createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
#ifdef NDEBUG
  createInfo.enabledLayerCount = 0;
  createInfo.pNext = nullptr;
#else
  createInfo.enabledLayerCount = ValidationLayers.size();
  createInfo.ppEnabledLayerNames = ValidationLayers.data();

  auto debugMessengerInfo = getDebugMessengerCreateInfo();
  createInfo.pNext = &debugMessengerInfo;
#endif

  auto extensions = getRequiredExtensions();

  createInfo.enabledExtensionCount = extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();

  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance), "Failed to create instance");
}

VkDebugUtilsMessengerCreateInfoEXT RendererImpl::getDebugMessengerCreateInfo() const
{
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;
  return createInfo;
}

void RendererImpl::setupDebugMessenger()
{
  auto createInfo = getDebugMessengerCreateInfo();

  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
    vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func == nullptr) {
    EXCEPTION("Error getting pointer to vkCreateDebugUtilsMessengerEXT()");
  }
  VK_CHECK(func(m_instance, &createInfo, nullptr, &m_debugMessenger),
    "Error setting up debug messenger");
}

void RendererImpl::destroyDebugMessenger()
{
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
    vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
  func(m_instance, m_debugMessenger, nullptr);
}

void RendererImpl::cleanUp()
{
  vkDeviceWaitIdle(m_device);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
  }
  vkDestroyCommandPool(m_device, m_commandPool, nullptr);
  m_pipelines.clear();
  vkDestroyRenderPass(m_device, m_renderPass, nullptr);
  cleanupSwapChain();
  m_resources.reset();
#ifndef NDEBUG
  destroyDebugMessenger();
#endif
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  vkDestroyDevice(m_device, nullptr);
  vkDestroyInstance(m_instance, nullptr);
}

RendererImpl::~RendererImpl()
{
  m_running = false;
}

} // namespace

RendererPtr createRenderer(GLFWwindow& window, Logger& logger)
{
  return std::make_unique<RendererImpl>(window, logger);
}

