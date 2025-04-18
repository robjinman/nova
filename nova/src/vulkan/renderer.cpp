#include "vulkan/vulkan_utils.hpp"
#include "vulkan/pipeline.hpp"
#include "vulkan/render_resources.hpp"
#include "vulkan/vulkan_window_delegate.hpp"
#include "renderer.hpp"
#include "exception.hpp"
#include "version.hpp"
#include "logger.hpp"
#include "camera.hpp"
#include "utils.hpp"
#include "time.hpp"
#include "thread.hpp"
#include "triple_buffer.hpp"
#include "trace.hpp"
#include <array>
#include <vector>
#include <algorithm>
#include <cstring>
#include <optional>
#include <fstream>
#include <atomic>
#include <set>
#include <map>
#include <cassert>
#include <iostream>

namespace
{

const std::vector<const char*> ValidationLayers = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DeviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
  "VK_KHR_portability_subset",
#endif
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
    RendererImpl(const FileSystem& fileSystem, VulkanWindowDelegate& window, Logger& logger);

    void start() override;
    void onResize() override;
    double frameRate() const override;
    const ViewParams& getViewParams() const override;
    void checkError() const override;

    // Initialisation
    //
    void compileShader(const MeshFeatureSet& meshFeatures,
      const MaterialFeatureSet& materialFeatures) override;

    // Resources
    //
    RenderItemId addTexture(TexturePtr texture) override;
    RenderItemId addNormalMap(TexturePtr texture) override;
    RenderItemId addCubeMap(std::array<TexturePtr, 6>&& textures) override;
    void removeTexture(RenderItemId id) override;
    void removeCubeMap(RenderItemId id) override;

    // Meshes
    //
    MeshHandle addMesh(MeshPtr mesh) override;
    void removeMesh(RenderItemId id) override;

    // Materials
    //
    MaterialHandle addMaterial(MaterialPtr material) override;
    void removeMaterial(RenderItemId id) override;

    // Per frame draw functions
    //
    void beginFrame(const Camera& camera) override;
    void drawInstance(MeshHandle mesh, MaterialHandle material, const Mat4x4f& transform) override;
    void drawModel(MeshHandle mesh, MaterialHandle material, const Mat4x4f& transform) override;
    void drawLight(const Vec3f& colour, float_t ambient, float_t specular,
      const Vec3f& worldPos) override;
    void drawSkybox(MeshHandle mesh, MaterialHandle material) override;
    void endFrame() override;

    ~RendererImpl() override;

  private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
      const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData);

    void createInstance();
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
    void createSwapChain(VkExtent2D extent);
    void recreateSwapChain();
    void setProjectionMatrix(float_t rotation);
    void cleanupSwapChain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createDepthResources();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void updateResources();
    void updateMatricesUbo(const Mat4x4f& cameraMatrix);
    void finishFrame();
    void createSyncObjects();
    VkFormat findDepthFormat() const;
    void renderLoop();
    void cleanUp();
    RenderGraph::Key generateRenderGraphKey(MeshHandle mesh, MaterialHandle material) const;

    ViewParams m_viewParams;
    const FileSystem& m_fileSystem;
    VulkanWindowDelegate& m_window;
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
    std::atomic<bool> m_framebufferResized = false;
    Mat4x4f m_projectionMatrix;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    struct RenderState
    {
      RenderGraph graph;
      std::map<RenderGraph::Key, RenderNode*> lookup;
      LightingUbo lighting;
      Vec3f cameraPos;
      Mat4x4f cameraMatrix;
    };

    TripleBuffer<RenderState> m_renderStates;
  
    RenderResourcesPtr m_resources;
    std::unordered_map<PipelineKey, PipelinePtr> m_pipelines;

    Timer m_timer;
    std::atomic<double> m_frameRate;

    Thread m_thread;
    std::atomic<bool> m_running;
    mutable std::mutex m_errorMutex;
    bool m_hasError = false;
    std::string m_error;
};

RendererImpl::RendererImpl(const FileSystem& fileSystem, VulkanWindowDelegate& window,
  Logger& logger)
  : m_fileSystem(fileSystem)
  , m_window(window)
  , m_logger(logger)
{
  DBG_TRACE(m_logger);

  m_viewParams = ViewParams{
    .hFov = 0.f,
    .vFov = degreesToRadians(45.f),
    .aspectRatio = 0,
    .nearPlane = 0.1f,
    .farPlane = 10000.f
  };

  m_thread.run<void>([this]() {
    createInstance();
#ifndef NDEBUG
    setupDebugMessenger();
#endif
  }).get();
  m_surface = m_window.createSurface(m_instance);
  m_thread.run<void>([this]() {
    pickPhysicalDevice();
    createLogicalDevice();
  }).get();
  createSwapChain();
  m_thread.run<void>([this]() {
    createImageViews();
    createRenderPass();
    createCommandPool();
    m_resources = createRenderResources(m_physicalDevice, m_device, m_graphicsQueue, m_commandPool,
      m_logger);
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
    createSyncObjects();
  }).get();
}

void RendererImpl::start()
{
  m_running = true;
  m_thread.run<void>([&]() {
    renderLoop();
  });
}

void RendererImpl::checkError() const
{
  std::lock_guard lock(m_errorMutex);

  if (m_hasError) {
    EXCEPTION(m_error);
  }
}

void RendererImpl::compileShader(const MeshFeatureSet& meshFeatures,
  const MaterialFeatureSet& materialFeatures)
{
  ASSERT(!m_running, "Renderer already started");

  return m_thread.run<void>([&, this]() {
    PipelineKey key{
      .meshFeatures = meshFeatures,
      .materialFeatures = materialFeatures
    };
  
    if (!m_pipelines.contains(key)) {
      auto pipeline = createPipeline(meshFeatures, materialFeatures, m_fileSystem, *m_resources,
        m_device, m_swapchainExtent, m_renderPass);

      m_pipelines.insert(std::make_pair(key, std::move(pipeline)));
    }
  }).get();
}

double RendererImpl::frameRate() const
{
  return m_frameRate;
}

void RendererImpl::onResize()
{
  m_framebufferResized = true;
}

const ViewParams& RendererImpl::getViewParams() const
{
  return m_viewParams;
}

RenderGraph::Key RendererImpl::generateRenderGraphKey(MeshHandle mesh,
  MaterialHandle material) const
{
  PipelineKey pipelineKey{ mesh.features, material.features };
  auto pipelineHash = std::hash<PipelineKey>{}(pipelineKey);

  if (mesh.features.isInstanced) {
    return RenderGraph::Key{
      static_cast<RenderGraphKey>(material.features.hasTransparency),
      static_cast<RenderGraphKey>(pipelineHash),
      static_cast<RenderGraphKey>(mesh.id),
      static_cast<RenderGraphKey>(material.id)
    };
  }
  else if (mesh.features.isSkybox) {
    return RenderGraph::Key{
      static_cast<RenderGraphKey>(material.features.hasTransparency),
      static_cast<RenderGraphKey>(pipelineHash)
    };
  }
  else {
    static RenderGraphKey nextId = 0;

    return RenderGraph::Key{
      static_cast<RenderGraphKey>(material.features.hasTransparency),
      static_cast<RenderGraphKey>(pipelineHash),
      static_cast<RenderGraphKey>(mesh.id),
      static_cast<RenderGraphKey>(material.id),
      nextId++
    };
  }
}

void RendererImpl::drawInstance(MeshHandle mesh, MaterialHandle material, const Mat4x4f& transform)
{
  //DBG_TRACE(m_logger);

  RenderState& state = m_renderStates.getWritable();
  RenderGraph& renderGraph = state.graph;

  auto key = generateRenderGraphKey(mesh, material);
  InstancedModelNode* node = nullptr;
  auto i = state.lookup.find(key);
  if (i != state.lookup.end()) {
    node = dynamic_cast<InstancedModelNode*>(i->second);
  }
  else {
    auto newNode = std::make_unique<InstancedModelNode>();
    newNode->mesh = mesh;
    newNode->material = material;
    node = newNode.get();
    renderGraph.insert(key, std::move(newNode));
    state.lookup.insert({ key, node });
  }
  node->instances.push_back(MeshInstance{transform * mesh.transform});
}

void RendererImpl::drawModel(MeshHandle mesh, MaterialHandle material, const Mat4x4f& transform)
{
  //DBG_TRACE(m_logger);

  RenderState& state = m_renderStates.getWritable();
  RenderGraph& renderGraph = state.graph;

  auto node = std::make_unique<DefaultModelNode>();
  node->mesh = mesh;
  node->material = material;
  node->modelMatrix = transform;

  auto key = generateRenderGraphKey(mesh, material);

  state.lookup.insert({ key, node.get() });
  renderGraph.insert(key, std::move(node));
}

void RendererImpl::drawLight(const Vec3f& colour, float_t ambient, float_t specular,
  const Vec3f& worldPos)
{
  auto& state = m_renderStates.getWritable();
  ASSERT(state.lighting.numLights < MAX_LIGHTS, "Exceeded max lights");

  Light& light = state.lighting.lights[state.lighting.numLights++];
  light.colour = colour;
  light.ambient = ambient;
  light.specular = specular;
  light.worldPos = worldPos;
}

void RendererImpl::drawSkybox(MeshHandle mesh, MaterialHandle material)
{
  //DBG_TRACE(m_logger);

  RenderState& state = m_renderStates.getWritable();
  RenderGraph& renderGraph = state.graph;

  auto node = std::make_unique<SkyboxNode>();
  node->mesh = mesh;
  node->material = material;

  auto key = generateRenderGraphKey(mesh, material);

  state.lookup.insert({ key, node.get() });
  renderGraph.insert(key, std::move(node));
}

void RendererImpl::beginFrame(const Camera& camera)
{
  DBG_TRACE(m_logger);

  auto& state = m_renderStates.getWritable();
  state.lookup.clear();
  state.graph.clear();
  state.lighting = LightingUbo{};
  state.cameraPos = camera.getPosition();
  state.cameraMatrix = camera.getMatrix();
}

void RendererImpl::renderLoop()
{
  try {
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

      auto& state = m_renderStates.getReadable();

      updateMatricesUbo(state.cameraMatrix);
      state.lighting.cameraPos = state.cameraPos;
      m_resources->updateLightingUbo(state.lighting, m_currentFrame);

      updateResources();
      recordCommandBuffer(m_commandBuffers[m_imageIndex], m_imageIndex);
      finishFrame();

      m_renderStates.readComplete();

      m_frameRate = 1.0 / m_timer.elapsed();
      m_timer.reset();
    }
  }
  catch (const std::exception& e) {
    std::lock_guard lock(m_errorMutex);

    m_hasError = true;
    m_error = e.what();
    m_running = false;
  }

  cleanUp();
}

void RendererImpl::updateResources()
{
  // TODO: Move this into recordCommandBuffer?

  auto& renderGraph = m_renderStates.getReadable().graph;

  for (auto& node : renderGraph) {
    switch (node->type) {
      case RenderNodeType::InstancedModel: {
        auto& nodeData = dynamic_cast<const InstancedModelNode&>(*node);
        m_resources->updateMeshInstances(nodeData.mesh.id, nodeData.instances);

        break;
      }
      default: break;
    }
  }
}

void RendererImpl::endFrame()
{
  DBG_TRACE(m_logger);

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

  VkSwapchainKHR swapchains[] = { m_swapchain };

  VkPresentInfoKHR presentInfo{
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = nullptr,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signalSemaphores,
    .swapchainCount = 1,
    .pSwapchains = swapchains,
    .pImageIndices = &m_imageIndex,
    .pResults = nullptr
  };

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

void RendererImpl::updateMatricesUbo(const Mat4x4f& cameraMatrix)
{
  DBG_TRACE(m_logger);

  MatricesUbo ubo{
    .viewMatrix = cameraMatrix,
    .projMatrix = m_projectionMatrix
  };

  m_resources->updateMatricesUbo(ubo, m_currentFrame);
}

void RendererImpl::removeMesh(RenderItemId)
{
  // TODO
  EXCEPTION("Not implemented");
  //m_resources->removeMesh(id);
}

void RendererImpl::removeTexture(RenderItemId)
{
  // TODO
  EXCEPTION("Not implemented");
  //m_resources->removeTexture(id);
}

void RendererImpl::removeCubeMap(RenderItemId)
{
  // TODO
  EXCEPTION("Not implemented");
  //m_resources->removeCubeMap(id);
}

void RendererImpl::removeMaterial(RenderItemId)
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
    m_window.getFrameBufferSize(width, height);

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
    if (format.format == VK_FORMAT_R16G16B16A16_SFLOAT &&
      format.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {

      return format;
    }
  }

  assert(availableFormats.size() > 0);

  m_logger.warn("Preferred swap chain surface format not available");

  return availableFormats[0];
}

void RendererImpl::createSwapChain()
{
  DBG_TRACE(m_logger);

  auto swapchainSupport = querySwapChainSupport(m_physicalDevice);
  auto extent = chooseSwapChainExtent(swapchainSupport.capabilities);
  createSwapChain(extent);
}

void RendererImpl::createSwapChain(VkExtent2D extent)
{
  auto swapchainSupport = querySwapChainSupport(m_physicalDevice);
  auto surfaceFormat = chooseSwapChainSurfaceFormat(swapchainSupport.formats);
  auto presentMode = chooseSwapChainPresentMode(swapchainSupport.presentModes);

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

  float_t rotation = 0;
  if (swapchainSupport.capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
    rotation = degreesToRadians(90.f);
  }
  setProjectionMatrix(rotation);
}

void RendererImpl::setProjectionMatrix(float_t rotation)
{
  float_t aspect = m_swapchainExtent.width / static_cast<float_t>(m_swapchainExtent.height);

  m_viewParams.aspectRatio = aspect;
  m_viewParams.hFov = 2.f * atan(aspect * tan(0.5f * m_viewParams.vFov));

  Mat4x4f rot = rotationMatrix4x4<float_t>(Vec3f{ 0.f, 0.f, rotation });
  m_projectionMatrix = rot * perspective(m_viewParams.hFov, m_viewParams.vFov,
    m_viewParams.nearPlane, m_viewParams.farPlane);
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
  m_window.getFrameBufferSize(width, height);

  VK_CHECK(vkDeviceWaitIdle(m_device), "Error waiting for device to be idle");

  VkExtent2D extent{
    .width = static_cast<uint32_t>(width),
    .height = static_cast<uint32_t>(height)
  };

  cleanupSwapChain();
  createSwapChain(extent);
  createImageViews();
  createDepthResources();
  createFramebuffers();

  for (auto& i : m_pipelines) {
    i.second->onViewportResize(m_swapchainExtent);
  }

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

#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
  extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  auto windowExtensions = m_window.getRequiredExtensions();
  extensions.insert(extensions.end(), windowExtensions.begin(), windowExtensions.end());

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
  DBG_TRACE(m_logger);

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

  VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
  indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
  indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;

  VkPhysicalDeviceFeatures2 deviceFeatures{};
  deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  //deviceFeatures.pNext = &indexingFeatures;
  deviceFeatures.features.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = &deviceFeatures;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = nullptr;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
  createInfo.ppEnabledExtensionNames = DeviceExtensions.data();

#ifdef NDEBUG
  createInfo.enabledLayerCount = 0;
#else
  createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
  createInfo.ppEnabledLayerNames = ValidationLayers.data();
#endif

  VK_CHECK(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device),
    "Failed to create logical device");

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

void RendererImpl::createImageViews()
{
  DBG_TRACE(m_logger);

  m_swapchainImageViews.resize(m_swapchainImages.size());

  for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
    m_swapchainImageViews[i] = createImageView(m_device, m_swapchainImages[i],
      m_swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);
  }
}

void RendererImpl::createRenderPass()
{
  DBG_TRACE(m_logger);

  VkAttachmentDescription colourAttachment{
    .flags = 0,
    .format = m_swapchainImageFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  };

  VkAttachmentReference colourAttachmentRef{
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };

  VkAttachmentDescription depthAttachment{
    .flags = 0,
    .format = findDepthFormat(),
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  };

  VkAttachmentReference depthAttachmentRef{
    .attachment = 1,
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  };

  VkSubpassDescription subpass{
    .flags = 0,
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .inputAttachmentCount = 0,
    .pInputAttachments = nullptr,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colourAttachmentRef,
    .pResolveAttachments = nullptr,
    .pDepthStencilAttachment = &depthAttachmentRef,
    .preserveAttachmentCount = 0,
    .pPreserveAttachments = nullptr,
  };

  std::array<VkAttachmentDescription, 2> attachments = {
    colourAttachment,
    depthAttachment
  };

  VkSubpassDependency dependency{
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
      | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
      | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
      | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .dependencyFlags = 0
  };

  VkRenderPassCreateInfo renderPassInfo{
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .attachmentCount = static_cast<uint32_t>(attachments.size()),
    .pAttachments = attachments.data(),
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency
  };

  VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass),
    "Failed to create render pass");
}

void RendererImpl::createFramebuffers()
{
  DBG_TRACE(m_logger);

  m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

  for (size_t i = 0; i < m_swapchainImageViews.size(); ++i) {
    std::array<VkImageView, 2> attachments = { m_swapchainImageViews[i], m_depthImageView };

    VkFramebufferCreateInfo framebufferInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderPass = m_renderPass,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .width = m_swapchainExtent.width,
      .height = m_swapchainExtent.height,
      .layers = 1
    };

    VK_CHECK(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]),
      "Failed to create framebuffer");
  }
}

void RendererImpl::createCommandPool()
{
  DBG_TRACE(m_logger);

  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);
  VkCommandPoolCreateInfo poolInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
  };

  VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool),
    "Failed to create command pool");
}

void RendererImpl::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
  VkCommandBufferBeginInfo beginInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = nullptr,
    .flags = 0,
    .pInheritanceInfo = nullptr
  };

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo),
    "Failed to begin recording command buffer");

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{ 0.0f, 0.0f, 1.0f, 1.0f }};
  clearValues[1].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo renderPassInfo{
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .pNext = nullptr,
    .renderPass = m_renderPass,
    .framebuffer = m_swapchainFramebuffers[imageIndex],
    .renderArea = VkRect2D{
      .offset = { 0, 0 },
      .extent = m_swapchainExtent
    },
    .clearValueCount = static_cast<uint32_t>(clearValues.size()),
    .pClearValues = clearValues.data()
  };

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  auto choosePipeline = [this](const RenderNode& node) -> Pipeline& {
    PipelineKey key{
      .meshFeatures = node.mesh.features,
      .materialFeatures = node.material.features
    };
    auto i = m_pipelines.find(key);
    if (i == m_pipelines.end()) {
      EXCEPTION("No shader has been compiled for this combination of mesh/material features");
    }
    return *i->second;
  };

  auto& state = m_renderStates.getReadable();
  const auto& renderGraph = state.graph;
  BindState bindState{};
  for (auto& node : renderGraph) {
    auto& pipeline = choosePipeline(*node);
    pipeline.recordCommandBuffer(commandBuffer, *node, bindState, m_currentFrame);
  }

  vkCmdEndRenderPass(commandBuffer);

  VK_CHECK(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer");
}

void RendererImpl::createCommandBuffers()
{
  DBG_TRACE(m_logger);

  m_commandBuffers.resize(m_swapchainFramebuffers.size());

  VkCommandBufferAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = nullptr,
    .commandPool = m_commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size())
  };

  VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()),
    "Failed to allocate command buffers");
}

RenderItemId RendererImpl::addTexture(TexturePtr texture)
{
  DBG_TRACE(m_logger);

  ASSERT(!m_running, "Renderer already started");
  return m_thread.run<RenderItemId>([&, this]() {
    return m_resources->addTexture(std::move(texture));
  }).get();
}

RenderItemId RendererImpl::addNormalMap(TexturePtr texture)
{
  DBG_TRACE(m_logger);

  ASSERT(!m_running, "Renderer already started");
  return m_thread.run<RenderItemId>([&, this]() {
    return m_resources->addNormalMap(std::move(texture));
  }).get();
}

RenderItemId RendererImpl::addCubeMap(std::array<TexturePtr, 6>&& textures)
{
  DBG_TRACE(m_logger);

  ASSERT(!m_running, "Renderer already started");
  return m_thread.run<RenderItemId>([&, this]() {
    return m_resources->addCubeMap(std::move(textures));
  }).get();
}

MaterialHandle RendererImpl::addMaterial(MaterialPtr material)
{
  DBG_TRACE(m_logger);

  ASSERT(!m_running, "Renderer already started");
  return m_thread.run<MaterialHandle>([&, this]() {
    return m_resources->addMaterial(std::move(material));
  }).get();
}

MeshHandle RendererImpl::addMesh(MeshPtr mesh)
{
  DBG_TRACE(m_logger);

  ASSERT(!m_running, "Renderer already started");
  return m_thread.run<MeshHandle>([&, this]() {
    return m_resources->addMesh(std::move(mesh));
  }).get();
}

void RendererImpl::createSyncObjects()
{
  DBG_TRACE(m_logger);

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
  DBG_TRACE(m_logger);

  VkFormat depthFormat = findDepthFormat();

  createImage(m_physicalDevice, m_device, m_swapchainExtent.width, m_swapchainExtent.height,
    depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);

  m_depthImageView = createImageView(m_device, m_depthImage, depthFormat,
    VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);
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
      index = static_cast<int>(dev.second);
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
  DBG_TRACE(m_logger);

#ifndef NDEBUG
  checkValidationLayerSupport();
#endif

  VkApplicationInfo appInfo{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = nullptr,
    .pApplicationName = "Project Nova",
    .applicationVersion = VK_MAKE_VERSION(Nova_VERSION_MAJOR, Nova_VERSION_MINOR, 0),
    .pEngineName = "No Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_0
  };

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
  createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
  createInfo.ppEnabledLayerNames = ValidationLayers.data();

  auto debugMessengerInfo = getDebugMessengerCreateInfo();
  createInfo.pNext = &debugMessengerInfo;
#endif

  auto extensions = getRequiredExtensions();

  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance), "Failed to create instance");
}

VkDebugUtilsMessengerCreateInfoEXT RendererImpl::getDebugMessengerCreateInfo() const
{
  return VkDebugUtilsMessengerCreateInfoEXT{
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pNext = nullptr,
    .flags = 0,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debugCallback,
    .pUserData = nullptr
  };
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

RendererPtr createRenderer(const FileSystem& fileSystem, WindowDelegate& window, Logger& logger)
{
  return std::make_unique<RendererImpl>(fileSystem, dynamic_cast<VulkanWindowDelegate&>(window),
    logger);
}
