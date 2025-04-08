#include "vulkan/render_resources.hpp"
#include "vulkan/vulkan_utils.hpp"
#include "logger.hpp"
#include "trace.hpp"
#include "utils.hpp"
#include <map>
#include <array>
#include <cstring>
#include <cassert>

namespace
{

struct MeshData
{
  MeshPtr mesh = nullptr;
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
  VkBuffer instanceBuffer = VK_NULL_HANDLE;
  VkDeviceMemory instanceBufferMemory = VK_NULL_HANDLE;
  uint32_t numInstances = 0;
};

using MeshDataPtr = std::unique_ptr<MeshData>;

struct TextureData
{
  TexturePtr texture;
  VkImage image;
  VkDeviceMemory imageMemory;
  VkImageView imageView;
};

using TextureDataPtr = std::unique_ptr<TextureData>;

struct CubeMapData
{
  std::array<TexturePtr, 6> textures;
  VkImage image;
  VkDeviceMemory imageMemory;
  VkImageView imageView;
};

using CubeMapDataPtr = std::unique_ptr<CubeMapData>;

struct MaterialData
{
  MaterialPtr material;
  VkDescriptorSet descriptorSet;
  VkBuffer uboBuffer;
  VkDeviceMemory uboMemory;
  void* uboMapped = nullptr;  // TOOD: Remove mapping to host memory
};

using MaterialDataPtr = std::unique_ptr<MaterialData>;

class RenderResourcesImpl : public RenderResources
{
  public:
    RenderResourcesImpl(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue,
      VkCommandPool commandPool, Logger& logger);

    // Resources
    //
    RenderItemId addTexture(TexturePtr texture) override;
    RenderItemId addCubeMap(std::array<TexturePtr, 6> textures) override;
    void removeTexture(RenderItemId id) override;
    void removeCubeMap(RenderItemId id) override;

    // Meshes
    //
    MeshHandle addMesh(MeshPtr mesh) override;
    void removeMesh(RenderItemId id) override;
    MeshBuffers getMeshBuffers(RenderItemId id) const override;
    void updateMeshInstances(RenderItemId id, const std::vector<MeshInstance>& instances) override;
    const MeshFeatureSet& getMeshFeatures(RenderItemId id) const override;

    // Materials
    //
    MaterialHandle addMaterial(MaterialPtr material) override;
    void removeMaterial(RenderItemId id) override;
    VkDescriptorSetLayout getMaterialDescriptorSetLayout() const override;
    VkDescriptorSet getMaterialDescriptorSet(RenderItemId id) const override;
    const MaterialFeatureSet& getMaterialFeatures(RenderItemId id) const override;

    // Matrices
    //
    void updateMatricesUbo(const MatricesUbo& ubo, size_t currentFrame) override;
    VkDescriptorSetLayout getMatricesDescriptorSetLayout() const override;
    VkDescriptorSet getMatricesDescriptorSet(size_t currentFrame) const override;

    // Lighting
    //
    void updateLightingUbo(const LightingUbo& ubo, size_t currentFrame) override;
    VkDescriptorSetLayout getLightingDescriptorSetLayout() const override;
    VkDescriptorSet getLightingDescriptorSet(size_t currentFrame) const override;

    ~RenderResourcesImpl() override;

  private:
    std::map<RenderItemId, MeshDataPtr> m_meshes;
    std::map<RenderItemId, TextureDataPtr> m_textures;
    std::map<RenderItemId, CubeMapDataPtr> m_cubeMaps;
    std::map<RenderItemId, MaterialDataPtr> m_materials;

    RenderItemId m_nullTextureId;
    RenderItemId m_nullMaterialId;

    Logger& m_logger;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkCommandPool m_commandPool;
    VkDescriptorPool m_descriptorPool;

    VkDescriptorSetLayout m_matricesUboDescriptorSetLayout;
    std::vector<VkDescriptorSet> m_matricesUboDescriptorSets;
    std::vector<VkBuffer> m_matricesUboBuffers;
    std::vector<VkDeviceMemory> m_matricesUboMemory;
    std::vector<void*> m_matricesUboMapped;
  
    VkDescriptorSetLayout m_materialDescriptorSetLayout;
    VkSampler m_textureSampler;

    VkDescriptorSetLayout m_lightingUboDescriptorSetLayout;
    std::vector<VkDescriptorSet> m_lightingUboDescriptorSets;
    std::vector<VkBuffer> m_lightingUboBuffers;
    std::vector<VkDeviceMemory> m_lightingUboMemory;
    std::vector<void*> m_lightingUboMapped;

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
      VkDeviceSize bufferOffset, uint32_t layer);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
      VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    VkBuffer createVertexBuffer(const Mesh& mesh, VkDeviceMemory& vertexBufferMemory);
    VkBuffer createInstanceBuffer(size_t maxInstances, VkDeviceMemory& instanceBufferMemory);
    void updateInstanceBuffer(const std::vector<MeshInstance>& instanceData, VkBuffer buffer);
    void createTextureSampler();
    VkBuffer createIndexBuffer(const Buffer& indexBuffer, VkDeviceMemory& indexBufferMemory);
    void createUniformBuffers();
    void createPerFrameUbos(size_t size, std::vector<VkBuffer>& buffers,
      std::vector<VkDeviceMemory>& memory, std::vector<void*>& mappings);
    void createUbo(size_t size, VkBuffer& buffer, VkDeviceMemory& memory, void*& mapping);
    void createDescriptorPool();
    void createDescriptorSets();
    void createDescriptorSets(size_t n, VkDescriptorSetLayout layout,
      std::vector<VkDescriptorSet>& descriptorSets, const std::vector<VkBuffer>& buffers);
    void createDescriptorSetLayouts();
    void createUboDescriptorSetLayout();
    void createLightingDescriptorSetLayout();
    void createMaterialDescriptorSetLayout();
    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
      uint32_t layerCount);
    void createNullMaterial();
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
};

RenderResourcesImpl::RenderResourcesImpl(VkPhysicalDevice physicalDevice, VkDevice device,
  VkQueue graphicsQueue, VkCommandPool commandPool, Logger& logger)
  : m_logger(logger)
  , m_physicalDevice(physicalDevice)
  , m_device(device)
  , m_graphicsQueue(graphicsQueue)
  , m_commandPool(commandPool)
{
  DBG_TRACE(m_logger);

  createDescriptorSetLayouts();
  createTextureSampler();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createNullMaterial();
}

RenderItemId RenderResourcesImpl::addTexture(TexturePtr texture)
{
  static RenderItemId nextTextureId = 1;

  auto textureData = std::make_unique<TextureData>();

  ASSERT(texture->data.size() % 4 == 0, "Texture data size should be multiple of 4");

  VkDeviceSize imageSize = texture->data.size();
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
    stagingBufferMemory);

  void* data = nullptr;
  vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, texture->data.data(), static_cast<size_t>(imageSize));
  vkUnmapMemory(m_device, stagingBufferMemory);

  createImage(m_physicalDevice, m_device, texture->width, texture->height, VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureData->image, textureData->imageMemory);

  transitionImageLayout(textureData->image, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

  copyBufferToImage(stagingBuffer, textureData->image, texture->width, texture->height, 0, 0);

  transitionImageLayout(textureData->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);

  textureData->imageView = createImageView(m_device, textureData->image, VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

  auto textureId = nextTextureId++;
  textureData->texture = std::move(texture);
  m_textures[textureId] = std::move(textureData);

  return textureId;
}

RenderItemId RenderResourcesImpl::addCubeMap(std::array<TexturePtr, 6> textures)
{
  auto cubeMapData = std::make_unique<CubeMapData>();

  VkDeviceSize imageSize = textures[0]->data.size();
  VkDeviceSize cubeMapSize = imageSize * 6;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(cubeMapSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
    stagingBufferMemory);

  uint32_t width = textures[0]->width;
  uint32_t height = textures[0]->height;

  uint8_t* data = nullptr;
  vkMapMemory(m_device, stagingBufferMemory, 0, cubeMapSize, 0, reinterpret_cast<void**>(&data));
  for (size_t i = 0; i < 6; ++i) {
    ASSERT(textures[i]->data.size() % 4 == 0, "Texture data size should be multiple of 4");
    ASSERT(textures[i]->width == width, "Cube map images should have same size");
    ASSERT(textures[i]->height == height, "Cube map images should have same size");

    size_t offset = i * imageSize;
    memcpy(data + offset, textures[i]->data.data(), static_cast<size_t>(imageSize));
  }
  vkUnmapMemory(m_device, stagingBufferMemory);

  createImage(m_physicalDevice, m_device, width, height, VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubeMapData->image, cubeMapData->imageMemory, 6,
    VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

  transitionImageLayout(cubeMapData->image, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);

  for (uint32_t i = 0; i < 6; ++i) {
    VkDeviceSize offset = i * imageSize;
    copyBufferToImage(stagingBuffer, cubeMapData->image, width, height, offset, i);
  }

  transitionImageLayout(cubeMapData->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);

  cubeMapData->imageView = createImageView(m_device, cubeMapData->image, VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, 6);

  static RenderItemId nextCubeMapId = 1;

  auto cubeMapId = nextCubeMapId++;
  cubeMapData->textures = std::move(textures);
  m_cubeMaps[cubeMapId] = std::move(cubeMapData);

  return cubeMapId;
}

void RenderResourcesImpl::removeTexture(RenderItemId id)
{
  auto i = m_textures.find(id);
  if (i == m_textures.end()) {
    return;
  }

  vkDestroyImageView(m_device, i->second->imageView, nullptr);
  vkDestroyImage(m_device, i->second->image, nullptr);
  vkFreeMemory(m_device, i->second->imageMemory, nullptr);

  m_textures.erase(i);
}

void RenderResourcesImpl::removeCubeMap(RenderItemId id)
{
  auto i = m_cubeMaps.find(id);
  if (i == m_cubeMaps.end()) {
    return;
  }

  vkDestroyImageView(m_device, i->second->imageView, nullptr);
  vkDestroyImage(m_device, i->second->image, nullptr);
  vkFreeMemory(m_device, i->second->imageMemory, nullptr);

  m_cubeMaps.erase(i);
}

MeshHandle RenderResourcesImpl::addMesh(MeshPtr mesh)
{
  static RenderItemId nextMeshId = 1;

  MeshHandle handle;
  handle.features = mesh->featureSet;

  auto data = std::make_unique<MeshData>();
  data->mesh = std::move(mesh);
  data->vertexBuffer = createVertexBuffer(*data->mesh, data->vertexBufferMemory);
  data->indexBuffer = createIndexBuffer(data->mesh->indexBuffer, data->indexBufferMemory);
  if (data->mesh->featureSet.isInstanced) {
    data->instanceBuffer = createInstanceBuffer(data->mesh->featureSet.maxInstances,
      data->instanceBufferMemory);
  }

  handle.id = nextMeshId++;
  m_meshes[handle.id] = std::move(data);

  return handle;
}

void RenderResourcesImpl::removeMesh(RenderItemId id)
{
  auto i = m_meshes.find(id);
  if (i == m_meshes.end()) {
    return;
  }

  vkDestroyBuffer(m_device, i->second->indexBuffer, nullptr);
  vkFreeMemory(m_device, i->second->indexBufferMemory, nullptr);
  vkDestroyBuffer(m_device, i->second->vertexBuffer, nullptr);
  vkFreeMemory(m_device, i->second->vertexBufferMemory, nullptr);
  if (i->second->instanceBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(m_device, i->second->instanceBuffer, nullptr);
    vkFreeMemory(m_device, i->second->instanceBufferMemory, nullptr);
  }

  m_meshes.erase(i);
}

MeshBuffers RenderResourcesImpl::getMeshBuffers(RenderItemId id) const
{
  auto& mesh = m_meshes.at(id);

  return {
    .vertexBuffer = mesh->vertexBuffer,
    .indexBuffer = mesh->indexBuffer,
    .instanceBuffer = mesh->instanceBuffer,
    .numIndices = static_cast<uint32_t>(mesh->mesh->indexBuffer.data.size() / sizeof(uint16_t)),
    .numInstances = mesh->numInstances
  };
}

void RenderResourcesImpl::updateMeshInstances(RenderItemId id,
  const std::vector<MeshInstance>& instances)
{
  DBG_TRACE(m_logger);

  auto& mesh = m_meshes.at(id);
  ASSERT(mesh->mesh->featureSet.isInstanced, "Can't instance a non-instanced mesh");
  ASSERT(instances.size() <= mesh->mesh->featureSet.maxInstances,
    "Max instances exceeded for this mesh");

  mesh->numInstances = static_cast<uint32_t>(instances.size());
  updateInstanceBuffer(instances, mesh->instanceBuffer);
}

const MeshFeatureSet& RenderResourcesImpl::getMeshFeatures(RenderItemId id) const
{
  return m_meshes.at(id)->mesh->featureSet;
}

MaterialHandle RenderResourcesImpl::addMaterial(MaterialPtr material)
{
  static RenderItemId nextMaterialId = 1;

  // TODO: Use descriptorBindingPartiallyBound feature provided by VK_EXT_descriptor_indexing
  // extension

  MaterialHandle handle;
  handle.features = material->featureSet;

  auto materialData = std::make_unique<MaterialData>();

  // TODO
  ASSERT(!(material->cubeMap.id != NULL_ID && material->texture.id != NULL_ID),
    "Currently, materials cannot have both a cube map and a texture");

  RenderItemId textureId = material->texture.id;
  bool usingNullTexture = false;
  bool usingCubeMap = material->cubeMap.id != NULL_ID;

  if (textureId == NULL_ID) {
    textureId = m_nullTextureId;
    usingNullTexture = true;
  }

  VkImageView imageView = VK_NULL_HANDLE;
  if (!usingCubeMap) {
    imageView = m_textures.at(textureId)->imageView;
  }
  else {
    imageView = m_cubeMaps.at(material->cubeMap.id)->imageView;
  }
  assert(imageView != VK_NULL_HANDLE);

  VkDescriptorSetAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = nullptr,
    .descriptorPool = m_descriptorPool,
    .descriptorSetCount = 1,
    .pSetLayouts = &m_materialDescriptorSetLayout
  };

  VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &materialData->descriptorSet),
    "Failed to allocate descriptor sets");

  VkDescriptorImageInfo imageInfo{
    .sampler = m_textureSampler,
    .imageView = imageView,
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  };

  VkWriteDescriptorSet samplerDescriptorWrite{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = nullptr,
    .dstSet = materialData->descriptorSet,
    .dstBinding = 0,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = &imageInfo,
    .pBufferInfo = nullptr,
    .pTexelBufferView = nullptr
  };

  createUbo(sizeof(MaterialUbo), materialData->uboBuffer, materialData->uboMemory,
    materialData->uboMapped);

  vkUpdateDescriptorSets(m_device, 1, &samplerDescriptorWrite, 0, nullptr);

  VkDescriptorBufferInfo bufferInfo{
    .buffer = materialData->uboBuffer,
    .offset = 0,
    .range = sizeof(MaterialUbo)
  };

  VkWriteDescriptorSet uboDescriptorWrite{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = nullptr,
    .dstSet = materialData->descriptorSet,
    .dstBinding = 1,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .pImageInfo = nullptr,
    .pBufferInfo = &bufferInfo,
    .pTexelBufferView = nullptr
  };

  vkUpdateDescriptorSets(m_device, 1, &uboDescriptorWrite, 0, nullptr);

  // TODO: Use staging buffer instead of host mapping
  MaterialUbo ubo{
    .colour = material->colour,
    .hasTexture = !usingNullTexture
  };

  memcpy(materialData->uboMapped, &ubo, sizeof(ubo));

  handle.id = nextMaterialId++;
  materialData->material = std::move(material);
  m_materials[handle.id] = std::move(materialData);

  return handle;
}

void RenderResourcesImpl::removeMaterial(RenderItemId id)
{
  auto i = m_materials.find(id);
  if (i == m_materials.end()) {
    return;
  }

  vkDestroyBuffer(m_device, i->second->uboBuffer, nullptr);
  vkFreeMemory(m_device, i->second->uboMemory, nullptr);

  m_materials.erase(i);
}

VkDescriptorSetLayout RenderResourcesImpl::getMaterialDescriptorSetLayout() const
{
  return m_materialDescriptorSetLayout;
}

VkDescriptorSet RenderResourcesImpl::getMaterialDescriptorSet(RenderItemId id) const
{
  return m_materials.at(id == NULL_ID ? m_nullMaterialId : id)->descriptorSet;
}

const MaterialFeatureSet& RenderResourcesImpl::getMaterialFeatures(RenderItemId id) const
{
  return m_materials.at(id)->material->featureSet;
}

void RenderResourcesImpl::updateMatricesUbo(const MatricesUbo& ubo, size_t currentFrame)
{
  memcpy(m_matricesUboMapped[currentFrame], &ubo, sizeof(ubo));
}

VkDescriptorSetLayout RenderResourcesImpl::getMatricesDescriptorSetLayout() const
{
  return m_matricesUboDescriptorSetLayout;
}

VkDescriptorSet RenderResourcesImpl::getMatricesDescriptorSet(size_t currentFrame) const
{
  return m_matricesUboDescriptorSets[currentFrame];
}

void RenderResourcesImpl::updateLightingUbo(const LightingUbo& ubo, size_t currentFrame)
{
  memcpy(m_lightingUboMapped[currentFrame], &ubo, sizeof(ubo));
}

VkDescriptorSetLayout RenderResourcesImpl::getLightingDescriptorSetLayout() const
{
  return m_lightingUboDescriptorSetLayout;
}

VkDescriptorSet RenderResourcesImpl::getLightingDescriptorSet(size_t currentFrame) const
{
  return m_lightingUboDescriptorSets[currentFrame];
}

void RenderResourcesImpl::createDescriptorPool()
{
  DBG_TRACE(m_logger);

  std::array<VkDescriptorPoolSize, 2> poolSizes{};

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = 50; // TODO

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = 20; // TODO

  VkDescriptorPoolCreateInfo poolInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .maxSets = 100, // TODO
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool),
    "Failed to create descriptor pool");
}

void RenderResourcesImpl::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
  uint32_t height, VkDeviceSize bufferOffset, uint32_t layer)
{
  DBG_TRACE(m_logger);

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{
    .bufferOffset = bufferOffset,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    .imageSubresource = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = 0,
      .baseArrayLayer = layer,
      .layerCount = 1
    },
    .imageOffset = { 0, 0, 0 },
    .imageExtent = { width, height, 1 }
  };

  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
    &region);

  endSingleTimeCommands(commandBuffer);
}

void RenderResourcesImpl::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  DBG_TRACE(m_logger);

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{
    .srcOffset = 0,
    .dstOffset = 0,
    .size = size
  };

  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

void RenderResourcesImpl::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
  DBG_TRACE(m_logger);

  VkBufferCreateInfo bufferInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr
  };

  VK_CHECK(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer), "Failed to create buffer");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = nullptr,
    .allocationSize = memRequirements.size,
    .memoryTypeIndex = findMemoryType(m_physicalDevice, memRequirements.memoryTypeBits, properties)
  };

  VK_CHECK(vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory),
    "Failed to allocate memory for buffer");

  vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

VkBuffer RenderResourcesImpl::createVertexBuffer(const Mesh& mesh,
  VkDeviceMemory& vertexBufferMemory)
{
  DBG_TRACE(m_logger);

  std::vector<char> vertices = createVertexArray(mesh);

  VkDeviceSize size = vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
    stagingBufferMemory);

  void* data = nullptr;
  vkMapMemory(m_device, stagingBufferMemory, 0, size, 0, &data);
  memcpy(data, vertices.data(), size);
  vkUnmapMemory(m_device, stagingBufferMemory);

  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

  copyBuffer(stagingBuffer, vertexBuffer, size);

  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);

  return vertexBuffer;
}

VkBuffer RenderResourcesImpl::createIndexBuffer(const Buffer& indexBuffer,
  VkDeviceMemory& indexBufferMemory)
{
  DBG_TRACE(m_logger);

  VkDeviceSize size = indexBuffer.data.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer, stagingBufferMemory);

  void* data = nullptr;
  vkMapMemory(m_device, stagingBufferMemory, 0, size, 0, &data);
  memcpy(data, indexBuffer.data.data(), size);
  vkUnmapMemory(m_device, stagingBufferMemory);

  VkBuffer buffer = VK_NULL_HANDLE;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, indexBufferMemory);

  copyBuffer(stagingBuffer, buffer, size);

  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);

  return buffer;
}

VkBuffer RenderResourcesImpl::createInstanceBuffer(size_t maxInstances,
  VkDeviceMemory& instanceBufferMemory)
{
  DBG_TRACE(m_logger);

  VkDeviceSize size = sizeof(MeshInstance) * maxInstances;

  VkBuffer buffer = VK_NULL_HANDLE;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, instanceBufferMemory);

  return buffer;
}

void RenderResourcesImpl::updateInstanceBuffer(const std::vector<MeshInstance>& instances,
  VkBuffer buffer)
{
  DBG_TRACE(m_logger);

  VkDeviceSize size = sizeof(MeshInstance) * instances.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
    stagingBufferMemory);

  // TODO: Consider partial updates?

  void* data = nullptr;
  vkMapMemory(m_device, stagingBufferMemory, 0, size, 0, &data);
  memcpy(data, instances.data(), size);
  vkUnmapMemory(m_device, stagingBufferMemory);

  copyBuffer(stagingBuffer, buffer, size);

  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void RenderResourcesImpl::createUbo(size_t size, VkBuffer& buffer, VkDeviceMemory& memory,
  void*& mapping)
{
  DBG_TRACE(m_logger);

  createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, memory);

  vkMapMemory(m_device, memory, 0, size, 0, &mapping);
}

void RenderResourcesImpl::createPerFrameUbos(size_t size, std::vector<VkBuffer>& buffers,
  std::vector<VkDeviceMemory>& memory, std::vector<void*>& mappings)
{
  DBG_TRACE(m_logger);

  buffers.resize(MAX_FRAMES_IN_FLIGHT);
  memory.resize(MAX_FRAMES_IN_FLIGHT);
  mappings.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffers[i],
      memory[i]);

    vkMapMemory(m_device, memory[i], 0, size, 0, &mappings[i]);
  }
}

void RenderResourcesImpl::createUniformBuffers()
{
  DBG_TRACE(m_logger);

  createPerFrameUbos(sizeof(MatricesUbo), m_matricesUboBuffers, m_matricesUboMemory,
    m_matricesUboMapped);
  createPerFrameUbos(sizeof(LightingUbo), m_lightingUboBuffers, m_lightingUboMemory,
    m_lightingUboMapped);
}

void RenderResourcesImpl::createDescriptorSets(size_t n, VkDescriptorSetLayout layout,
  std::vector<VkDescriptorSet>& descriptorSets, const std::vector<VkBuffer>& buffers)
{
  DBG_TRACE(m_logger);

  std::vector<VkDescriptorSetLayout> layouts(n, layout);

  VkDescriptorSetAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = nullptr,
    .descriptorPool = m_descriptorPool,
    .descriptorSetCount = static_cast<uint32_t>(n),
    .pSetLayouts = layouts.data()
  };

  descriptorSets.resize(n);
  VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, descriptorSets.data()),
    "Failed to allocate descriptor sets");

  for (size_t i = 0; i < n; ++i) {
    VkDescriptorBufferInfo bufferInfo{
      .buffer = buffers[i],
      .offset = 0,
      .range = sizeof(MatricesUbo)
    };

    VkWriteDescriptorSet descriptorWrite{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = descriptorSets[i],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pImageInfo = nullptr,
      .pBufferInfo = &bufferInfo,
      .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
  }
}

void RenderResourcesImpl::createDescriptorSets()
{
  DBG_TRACE(m_logger);

  createDescriptorSets(MAX_FRAMES_IN_FLIGHT, m_matricesUboDescriptorSetLayout,
    m_matricesUboDescriptorSets, m_matricesUboBuffers);

  createDescriptorSets(MAX_FRAMES_IN_FLIGHT, m_lightingUboDescriptorSetLayout,
    m_lightingUboDescriptorSets, m_lightingUboBuffers);
}

VkCommandBuffer RenderResourcesImpl::beginSingleTimeCommands()
{
  VkCommandBufferAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = nullptr,
    .commandPool = m_commandPool, // TODO: Separate pool for temp buffers?
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };
  
  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = nullptr,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = nullptr
  };

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void RenderResourcesImpl::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = nullptr,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = nullptr,
    .pWaitDstStageMask = nullptr,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
    .signalSemaphoreCount = 0,
    .pSignalSemaphores = nullptr
  };
  
  vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(m_graphicsQueue); // TODO: Submit commands asynchronously (see p201)

  vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void RenderResourcesImpl::transitionImageLayout(VkImage image, VkImageLayout oldLayout,
  VkImageLayout newLayout, uint32_t layerCount)
{
  DBG_TRACE(m_logger);

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .pNext = nullptr,
    .srcAccessMask = 0,
    .dstAccessMask = 0,
    .oldLayout = oldLayout,
    .newLayout = newLayout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = layerCount
    }
  };

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
    newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else {
    EXCEPTION("Unsupported layout transition");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
    &barrier);

  endSingleTimeCommands(commandBuffer);
}

void RenderResourcesImpl::createUboDescriptorSetLayout()
{
  DBG_TRACE(m_logger);

  VkDescriptorSetLayoutBinding uboLayoutBinding{
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .pImmutableSamplers = nullptr
  };

  std::vector<VkDescriptorSetLayoutBinding> bindings{
    uboLayoutBinding
    // ...
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .bindingCount = static_cast<uint32_t>(bindings.size()),
    .pBindings = bindings.data()
  };

  VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr,
    &m_matricesUboDescriptorSetLayout), "Failed to create descriptor set layout");
}

void RenderResourcesImpl::createLightingDescriptorSetLayout()
{
  DBG_TRACE(m_logger);

  VkDescriptorSetLayoutBinding lightingLayoutBinding{
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    .pImmutableSamplers = nullptr
  };

  std::vector<VkDescriptorSetLayoutBinding> bindings{
    lightingLayoutBinding
    // ...
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .bindingCount = static_cast<uint32_t>(bindings.size()),
    .pBindings = bindings.data()
  };
  
  VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr,
    &m_lightingUboDescriptorSetLayout), "Failed to create descriptor set layout");
}

void RenderResourcesImpl::createMaterialDescriptorSetLayout()
{
  DBG_TRACE(m_logger);

  VkDescriptorSetLayoutBinding samplerLayoutBinding{
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    .pImmutableSamplers = nullptr
  };

  VkDescriptorSetLayoutBinding materialLayoutBinding{
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    .pImmutableSamplers = nullptr
  };

  std::vector<VkDescriptorSetLayoutBinding> bindings{
    samplerLayoutBinding,
    materialLayoutBinding
    // ...
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .bindingCount = static_cast<uint32_t>(bindings.size()),
    .pBindings = bindings.data()
  };

  VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr,
    &m_materialDescriptorSetLayout), "Failed to create descriptor set layout");
}

void RenderResourcesImpl::createDescriptorSetLayouts()
{
  DBG_TRACE(m_logger);

  createUboDescriptorSetLayout();
  createLightingDescriptorSetLayout();
  createMaterialDescriptorSetLayout();
}

void RenderResourcesImpl::createTextureSampler()
{
  DBG_TRACE(m_logger);

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

  VkSamplerCreateInfo samplerInfo{
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .mipLodBias = 0.f,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .minLod = 0.f,
    .maxLod = 0.f,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE
  };

  VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler),
    "Failed to create texture sampler");
}

void RenderResourcesImpl::createNullMaterial()
{
  DBG_TRACE(m_logger);

  TexturePtr texture = std::make_unique<Texture>();
  texture->channels = 3;
  texture->width = 1;
  texture->height = 1;
  texture->data = { 0, 0, 0, 0 };
  m_nullTextureId = addTexture(std::move(texture));

  MaterialPtr material = std::make_unique<Material>(MaterialFeatureSet{
    .hasTransparency = false,
    .hasTexture = true,
    .hasNormalMap = false
  });
  material->texture.id = NULL_ID;
  material->normalMap.id = NULL_ID;

  m_nullMaterialId = addMaterial(std::move(material)).id;
}

RenderResourcesImpl::~RenderResourcesImpl()
{
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyBuffer(m_device, m_matricesUboBuffers[i], nullptr);
    vkFreeMemory(m_device, m_matricesUboMemory[i], nullptr);

    vkDestroyBuffer(m_device, m_lightingUboBuffers[i], nullptr);
    vkFreeMemory(m_device, m_lightingUboMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_matricesUboDescriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_materialDescriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_lightingUboDescriptorSetLayout, nullptr);

  while (!m_meshes.empty()) {
    removeMesh(m_meshes.begin()->first);
  }
  vkDestroySampler(m_device, m_textureSampler, nullptr);
  while (!m_materials.empty()) {
    removeMaterial(m_materials.begin()->first);
  }
  while (!m_textures.empty()) {
    removeTexture(m_textures.begin()->first);
  }
  while (!m_cubeMaps.empty()) {
    removeCubeMap(m_cubeMaps.begin()->first);
  }
}

} // namespace

RenderResourcesPtr createRenderResources(VkPhysicalDevice physicalDevice, VkDevice device,
  VkQueue graphicsQueue, VkCommandPool commandPool, Logger& logger)
{
  return std::make_unique<RenderResourcesImpl>(physicalDevice, device, graphicsQueue, commandPool,
    logger);
}
