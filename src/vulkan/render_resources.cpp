#include "vulkan/render_resources.hpp"
#include "vulkan/vulkan_utils.hpp"
#include <map>
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
  size_t numInstances = 0;
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
};

using MaterialDataPtr = std::unique_ptr<MaterialData>;

class RenderResourcesImpl : public RenderResources
{
  public:
    RenderResourcesImpl(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue,
      VkCommandPool commandPool);

    // Resources
    //
    RenderItemId addTexture(TexturePtr texture) override;
    RenderItemId addCubeMap(std::array<TexturePtr, 6> textures) override;
    void removeTexture(RenderItemId id) override;
    void removeCubeMap(RenderItemId id) override;

    // Meshes
    //
    RenderItemId addMesh(MeshPtr mesh) override;
    void removeMesh(RenderItemId id) override;
    MeshBuffers getMeshBuffers(RenderItemId id) const;
    void updateMeshInstances(RenderItemId id, const std::vector<MeshInstance>& instances) override;

    // Materials
    //
    RenderItemId addMaterial(MaterialPtr material) override;
    void removeMaterial(RenderItemId id) override;
    VkDescriptorSetLayout getMaterialDescriptorSetLayout() const override;
    VkDescriptorSet getMaterialDescriptorSet(RenderItemId id) const override;

    // UBO
    //
    void updateUniformBuffer(const UniformBufferObject& ubo, size_t currentFrame) override;
    VkDescriptorSetLayout getUboDescriptorSetLayout() const override;
    VkDescriptorSet getUboDescriptorSet(size_t currentFrame) const override;

    ~RenderResourcesImpl() override;

  private:
    std::map<RenderItemId, MeshDataPtr> m_meshes;
    std::map<RenderItemId, TextureDataPtr> m_textures;
    std::map<RenderItemId, CubeMapDataPtr> m_cubeMaps;
    std::map<RenderItemId, MaterialDataPtr> m_materials;
    RenderItemId m_nullMaterialId;

    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkCommandPool m_commandPool;
    VkDescriptorPool m_descriptorPool;

    VkDescriptorSetLayout m_uboDescriptorSetLayout;
    std::vector<VkDescriptorSet> m_uboDescriptorSets;
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;
  
    VkDescriptorSetLayout m_materialDescriptorSetLayout;
    VkSampler m_textureSampler;

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
      VkDeviceSize bufferOffset, uint32_t layer);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
      VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    VkBuffer createVertexBuffer(const VertexList& vertices, VkDeviceMemory& vertexBufferMemory);
    VkBuffer createInstanceBuffer(size_t maxInstances, VkDeviceMemory& instanceBufferMemory);
    void updateInstanceBuffer(const std::vector<MeshInstance>& instanceData, VkBuffer buffer);
    void createTextureSampler();
    VkBuffer createIndexBuffer(const IndexList& indices, VkDeviceMemory& indexBufferMemory);
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createDescriptorSetLayouts();
    void createUboDescriptorSetLayout();
    void createMaterialDescriptorSetLayout();
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
      VkImageLayout newLayout, uint32_t layerCount);
    void createNullMaterial();
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
};

RenderResourcesImpl::RenderResourcesImpl(VkPhysicalDevice physicalDevice, VkDevice device,
  VkQueue graphicsQueue, VkCommandPool commandPool)
  : m_physicalDevice(physicalDevice)
  , m_device(device)
  , m_graphicsQueue(graphicsQueue)
  , m_commandPool(commandPool)
{
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

  transitionImageLayout(textureData->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

  copyBufferToImage(stagingBuffer, textureData->image, texture->width, texture->height, 0, 0);

  transitionImageLayout(textureData->image, VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

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

  void* data = nullptr;
  vkMapMemory(m_device, stagingBufferMemory, 0, cubeMapSize, 0, &data);
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

  transitionImageLayout(cubeMapData->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);

  for (size_t i = 0; i < 6; ++i) {
    VkDeviceSize offset = i * imageSize;
    copyBufferToImage(stagingBuffer, cubeMapData->image, width, height, offset, i);
  }

  transitionImageLayout(cubeMapData->image, VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

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

RenderItemId RenderResourcesImpl::addMesh(MeshPtr mesh)
{
  static RenderItemId nextMeshId = 1;

  auto data = std::make_unique<MeshData>();
  data->mesh = std::move(mesh);
  data->vertexBuffer = createVertexBuffer(data->mesh->vertices, data->vertexBufferMemory);
  data->indexBuffer = createIndexBuffer(data->mesh->indices, data->indexBufferMemory);
  data->instanceBuffer = createInstanceBuffer(data->mesh->maxInstances, data->instanceBufferMemory);

  RenderItemId id = nextMeshId++;
  m_meshes[id] = std::move(data);

  return id;
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
  vkDestroyBuffer(m_device, i->second->instanceBuffer, nullptr);
  vkFreeMemory(m_device, i->second->instanceBufferMemory, nullptr);

  m_meshes.erase(i);
}

MeshBuffers RenderResourcesImpl::getMeshBuffers(RenderItemId id) const
{
  auto& mesh = m_meshes.at(id);

  return {
    .vertexBuffer = mesh->vertexBuffer,
    .indexBuffer = mesh->indexBuffer,
    .instanceBuffer = mesh->instanceBuffer,
    .numIndices = mesh->mesh->indices.size(),
    .numInstances = mesh->numInstances
  };
}

void RenderResourcesImpl::updateMeshInstances(RenderItemId id,
  const std::vector<MeshInstance>& instances)
{
  auto& mesh = m_meshes.at(id);
  ASSERT(mesh->mesh->isInstanced, "Can't instance a non-instanced mesh");
  ASSERT(instances.size() <= mesh->mesh->maxInstances, "Max instances exceeded for this mesh");

  mesh->numInstances = instances.size();
  updateInstanceBuffer(instances, mesh->instanceBuffer);
}

RenderItemId RenderResourcesImpl::addMaterial(MaterialPtr material)
{
  static RenderItemId nextMaterialId = 1;

  // TODO: Use descriptorBindingPartiallyBound feature provided by VK_EXT_descriptor_indexing
  // extension

  auto materialData = std::make_unique<MaterialData>();

  // TODO
  ASSERT(!(material->cubeMap != NULL_ID && material->texture != NULL_ID),
    "Currently, materials must have either texture or cube map, but not both");

  VkImageView imageView = VK_NULL_HANDLE;
  if (material->texture != NULL_ID) {
    imageView = m_textures.at(material->texture)->imageView;
  }
  else if (material->cubeMap != NULL_ID) {
    imageView = m_cubeMaps.at(material->cubeMap)->imageView;
  }
  assert(imageView != VK_NULL_HANDLE);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = m_descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &m_materialDescriptorSetLayout;

  VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &materialData->descriptorSet),
    "Failed to allocate descriptor sets");

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = imageView;
  imageInfo.sampler = m_textureSampler;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = materialData->descriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);

  auto materialId = nextMaterialId++;
  materialData->material = std::move(material);
  m_materials[materialId] = std::move(materialData);

  return materialId;
}

void RenderResourcesImpl::removeMaterial(RenderItemId id)
{
  // TODO
  EXCEPTION("Not implemented");
}

VkDescriptorSetLayout RenderResourcesImpl::getMaterialDescriptorSetLayout() const
{
  return m_materialDescriptorSetLayout;
}

VkDescriptorSet RenderResourcesImpl::getMaterialDescriptorSet(RenderItemId id) const
{
  return m_materials.at(id == NULL_ID ? m_nullMaterialId : id)->descriptorSet;
}

void RenderResourcesImpl::updateUniformBuffer(const UniformBufferObject& ubo, size_t currentFrame)
{
  memcpy(m_uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

VkDescriptorSetLayout RenderResourcesImpl::getUboDescriptorSetLayout() const
{
  return m_uboDescriptorSetLayout;
}

VkDescriptorSet RenderResourcesImpl::getUboDescriptorSet(size_t currentFrame) const
{
  return m_uboDescriptorSets[currentFrame];
}

void RenderResourcesImpl::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = 10; // TODO

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 10; // TODO

  VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool),
    "Failed to create descriptor pool");
}

void RenderResourcesImpl::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
  uint32_t height, VkDeviceSize bufferOffset, uint32_t layer)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = bufferOffset;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = layer;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = { 0, 0, 0 };
  region.imageExtent = { width, height, 1 };

  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
    &region);

  endSingleTimeCommands(commandBuffer);
}

void RenderResourcesImpl::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

void RenderResourcesImpl::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferInfo.flags = 0;

  VK_CHECK(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer), "Failed to create buffer");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(m_physicalDevice, memRequirements.memoryTypeBits,
    properties);

  VK_CHECK(vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory),
    "Failed to allocate memory for buffer");

  vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

VkBuffer RenderResourcesImpl::createVertexBuffer(const VertexList& vertices,
  VkDeviceMemory& vertexBufferMemory)
{
  VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

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

VkBuffer RenderResourcesImpl::createIndexBuffer(const IndexList& indices,
  VkDeviceMemory& indexBufferMemory)
{
  VkDeviceSize size = sizeof(indices[0]) * indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer, stagingBufferMemory);

  void* data = nullptr;
  vkMapMemory(m_device, stagingBufferMemory, 0, size, 0, &data);
  memcpy(data, indices.data(), size);
  vkUnmapMemory(m_device, stagingBufferMemory);

  VkBuffer indexBuffer = VK_NULL_HANDLE;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

  copyBuffer(stagingBuffer, indexBuffer, size);
  
  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);

  return indexBuffer;
}

VkBuffer RenderResourcesImpl::createInstanceBuffer(size_t maxInstances,
  VkDeviceMemory& instanceBufferMemory)
{
  VkDeviceSize size = sizeof(MeshInstance) * maxInstances;

  VkBuffer buffer = VK_NULL_HANDLE;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, instanceBufferMemory);

  return buffer;
}

void RenderResourcesImpl::updateInstanceBuffer(const std::vector<MeshInstance>& instances,
  VkBuffer buffer)
{
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

void RenderResourcesImpl::createUniformBuffers()
{
  VkDeviceSize size = sizeof(UniformBufferObject);

  m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
  
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      m_uniformBuffers[i], m_uniformBuffersMemory[i]);

    vkMapMemory(m_device, m_uniformBuffersMemory[i], 0, size, 0, &m_uniformBuffersMapped[i]);
  }
}

void RenderResourcesImpl::createDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_uboDescriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = m_descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  m_uboDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, m_uboDescriptorSets.data()),
    "Failed to allocate descriptor sets");

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_uboDescriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
  }
}

VkCommandBuffer RenderResourcesImpl::beginSingleTimeCommands()
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = m_commandPool; // TODO: Separate pool for temp buffers?
  allocInfo.commandBufferCount = 1;
  
  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void RenderResourcesImpl::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);
  
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  
  vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(m_graphicsQueue); // TODO: Submit commands asynchronously (see p201)

  vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void RenderResourcesImpl::transitionImageLayout(VkImage image, VkFormat format,
  VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = layerCount;

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
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
    uboLayoutBinding
    // ...
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = bindings.size();
  layoutInfo.pBindings = bindings.data();
  
  VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_uboDescriptorSetLayout),
    "Failed to create descriptor set layout");
}

void RenderResourcesImpl::createMaterialDescriptorSetLayout()
{
  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 0;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers = nullptr;

  std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
    samplerLayoutBinding
    // ...
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = bindings.size();
  layoutInfo.pBindings = bindings.data();

  VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr,
    &m_materialDescriptorSetLayout), "Failed to create descriptor set layout");
}

void RenderResourcesImpl::createDescriptorSetLayouts()
{
  createUboDescriptorSetLayout();
  createMaterialDescriptorSetLayout();
}

void RenderResourcesImpl::createTextureSampler()
{
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.f;
  samplerInfo.minLod = 0.f;
  samplerInfo.maxLod = 0.f;

  VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler),
    "Failed to create texture sampler");
}

void RenderResourcesImpl::createNullMaterial()
{
  TexturePtr texture = std::make_unique<Texture>();
  texture->channels = 3;
  texture->width = 1;
  texture->height = 1;
  texture->data = { 0, 0, 0, 0 };
  auto textureId = addTexture(std::move(texture));

  MaterialPtr material = std::make_unique<Material>();
  material->texture = textureId;
  material->normalMap = NULL_ID;

  m_nullMaterialId = addMaterial(std::move(material));
}

RenderResourcesImpl::~RenderResourcesImpl()
{
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
    vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
  }
  vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_uboDescriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_materialDescriptorSetLayout, nullptr);
  while (!m_meshes.empty()) {
    removeMesh(m_meshes.begin()->first);
  }
  vkDestroySampler(m_device, m_textureSampler, nullptr);
  while (!m_textures.empty()) {
    removeTexture(m_textures.begin()->first);
  }
  while (!m_cubeMaps.empty()) {
    removeCubeMap(m_cubeMaps.begin()->first);
  }
}

} // namespace

RenderResourcesPtr createRenderResources(VkPhysicalDevice physicalDevice, VkDevice device,
  VkQueue graphicsQueue, VkCommandPool commandPool)
{
  return std::make_unique<RenderResourcesImpl>(physicalDevice, device, graphicsQueue, commandPool);
}
