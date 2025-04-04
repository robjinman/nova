#include "model.hpp"
#include "utils.hpp"
#include "exception.hpp"
#include "file_system.hpp"
#include "gltf.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fstream>
#include <cassert>
#include <iostream> // TODO

std::ostream& operator<<(std::ostream& stream, const Vertex& vertex)
{
  stream << "Vertex{ pos: (" << vertex.pos << "), normal: (" << vertex.normal << "), tex coord: ("
    << vertex.texCoord << ") }";

  return stream;
}

TexturePtr loadTexture(const std::vector<char>& data)
{
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data.data()),
    static_cast<int>(data.size()), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    EXCEPTION("Failed to load texture image");
  }

  TexturePtr texture = std::make_unique<Texture>();
  texture->width = width;
  texture->height = height;
  texture->channels = channels;
  texture->data.resize(width * height * channels);
  memcpy(texture->data.data(), pixels, width * height * channels);

  stbi_image_free(pixels);

  return texture;
}

MeshPtr cuboid(float_t W, float_t H, float_t D, const Vec2f& textureSize)
{
  float_t w = W / 2.f;
  float_t h = H / 2.f;
  float_t d = D / 2.f;

  float_t u = textureSize[0];
  float_t v = textureSize[1];

  MeshPtr mesh = std::make_unique<Mesh>();
  // Viewed from above
  //
  // A +---+ B
  //   |   |
  // D +---+ C
  //
  mesh->vertices = {
    // Bottom face
    {{ -w, -h, -d }, { 0, -1, 0 }, { 0, 0 }},         // A  0
    {{ w, -h, -d }, { 0, -1, 0 }, { W / u, 0 }},      // B  1
    {{ w, -h, d }, { 0, -1, 0 }, { W /u, D / v }},    // C  2
    {{ -w, -h, d }, { 0, -1, 0 }, { 0, D / v }},      // D  3

    // Top face
    {{ -w, h, d }, { 0, 1, 0 }, { 0, D / v }},        // D' 4
    {{ w, h, d }, { 0, 1, 0 }, { W / u, D / v }},     // C' 5
    {{ w, h, -d }, { 0, 1, 0 }, { W / u, 0 }},        // B' 6
    {{ -w, h, -d }, { 0, 1, 0 }, { 0, 0 }},           // A' 7

    // Right face
    {{ w, -h, d }, { 1, 0, 0 }, { D / u, 0 }},        // C  8
    {{ w, -h, -d }, { 1, 0, 0 }, { 0, 0 }},           // B  9
    {{ w, h, -d }, { 1, 0, 0 }, { 0, H / v }},        // B' 10
    {{ w, h, d }, { 1, 0, 0 }, { D / u, H / v }},     // C' 11

    // Left face
    {{ -w, -h, -d }, { -1, 0, 0 }, { 0, 0 }},         // A  12
    {{ -w, -h, d }, { -1, 0, 0 }, { D / u, 0 }},      // D  13
    {{ -w, h, d }, { -1, 0, 0 }, { D / u, H / v }},   // D' 14
    {{ -w, h, -d }, { -1, 0, 0 }, { 0, H / v }},      // A' 15

    // Far face
    {{ -w, -h, -d }, { 0, 0, -1 }, { 0, 0 }},         // A  16
    {{ -w, h, -d }, { 0, 0, -1 }, { 0, H / v }},      // A' 17
    {{ w, h, -d }, { 0, 0, -1 }, { W / u, H / v }},   // B' 18
    {{ w, -h, -d }, { 0, 0, -1 }, { W / u, 0 }},      // B  19

    // Near face
    {{ -w, -h, d }, { 0, 0, 1 }, { 0, 0 }},           // D  20
    {{ w, -h, d }, { 0, 0, 1 }, { W / u, 0 }},        // C  21
    {{ w, h, d }, { 0, 0, 1 }, { W / u, H / v }},     // C' 22
    {{ -w, h, d }, { 0, 0, 1 }, { 0, H / v }},        // D' 23
  };
  mesh->indices = {
    0, 1, 2, 0, 2, 3,         // Bottom face
    4, 5, 6, 4, 6, 7,         // Top face
    8, 9, 10, 8, 10, 11,      // Left face
    12, 13, 14, 12, 14, 15,   // Right face
    16, 17, 18, 16, 18, 19,   // Near face
    20, 21, 22, 20, 22, 23,   // Far face
  };

  return mesh;
}

template<typename T>
T convert(const char* value, gltf::ComponentType dataType)
{
  switch (dataType) {
    case gltf::ComponentType::SignedByte:
      return static_cast<T>(*reinterpret_cast<const int8_t*>(value));
    case gltf::ComponentType::UnsignedByte:
      return static_cast<T>(*reinterpret_cast<const uint8_t*>(value));
    case gltf::ComponentType::SignedShort:
      return static_cast<T>(*reinterpret_cast<const int16_t*>(value));
    case gltf::ComponentType::UnsignedShort:
      return static_cast<T>(*reinterpret_cast<const uint16_t*>(value));
    case gltf::ComponentType::UnsignedInt:
      return static_cast<T>(*reinterpret_cast<const uint32_t*>(value));
    case gltf::ComponentType::Float:
      return static_cast<T>(*reinterpret_cast<const float*>(value));
  }
}

template<typename T>
void convert(const char* src, gltf::ComponentType srcType, uint32_t n, T* dest)
{
  for (uint32_t i = 0; i < n; ++i) {
    *(dest + i) = convert<T>(src + i * getSize(srcType), srcType);
  }
}

template<typename T>
void copyToBuffer(const std::vector<std::vector<char>>& srcBuffers, char* dstBuffer,
  size_t structSize, size_t offsetIntoStruct, const gltf::BufferDesc& desc)
{
  const char* src = srcBuffers[desc.index].data() + desc.offset;
  for (unsigned long i = 0; i < desc.size; ++i) {
    size_t srcElemSize = getSize(desc.componentType) * desc.dimensions;
    T* dstPtr = reinterpret_cast<T*>(dstBuffer + i * structSize + offsetIntoStruct);
    convert<T>(src + i * srcElemSize, desc.componentType, desc.dimensions, dstPtr);
  }
}

MeshPtr constructMesh(const gltf::MeshDesc& meshDesc,
  const std::vector<std::vector<char>>& dataBuffers)
{
  auto mesh = std::make_unique<Mesh>();

  auto getOffset = [](gltf::ElementType type) {
    switch (type) {
      case gltf::ElementType::AttrPosition: return offsetof(Vertex, pos);
      case gltf::ElementType::AttrNormal: return offsetof(Vertex, normal);
      case gltf::ElementType::AttrTexCoord: return offsetof(Vertex, texCoord);
      // TODO: Joints and weights
      default: return 0ul;
    }
  };

  for (const auto& bufferDesc : meshDesc.buffers) {
    if (bufferDesc.type == gltf::ElementType::VertexIndex) {
      mesh->indices.resize(bufferDesc.size);
      copyToBuffer<uint16_t>(dataBuffers, reinterpret_cast<char*>(mesh->indices.data()),
        sizeof(uint16_t), 0, bufferDesc);
    }
    else {
      if (mesh->vertices.size() == 0) {
        mesh->vertices.resize(bufferDesc.size);
      }
      else {
        ASSERT(mesh->vertices.size() == bufferDesc.size,
          "Expected mesh to contain same number of each attribute");
      }
      copyToBuffer<float_t>(dataBuffers, reinterpret_cast<char*>(mesh->vertices.data()),
        sizeof(Vertex), getOffset(bufferDesc.type), bufferDesc);
    }
  }

  return mesh;
}

MaterialPtr constructMaterial(const gltf::MaterialDesc& materialDesc)
{
  auto material = std::make_unique<Material>();

  material->texture.fileName = materialDesc.baseColourTexture;
  material->normalMap.fileName = materialDesc.normalMap;
  material->colour = materialDesc.baseColourFactor;
  // TODO: PBR attributes

  return material;
}

ModelPtr loadModel(const FileSystem& fileSystem, const std::string& filePath)
{
  auto modelDesc = gltf::extractModel(fileSystem.readFile(filePath));

  std::vector<std::vector<char>> dataBuffers;
  for (const auto& buffer : modelDesc.buffers) {
    auto binPath = std::filesystem::path{filePath}.parent_path() / buffer;
    dataBuffers.push_back(fileSystem.readFile(binPath));
  }

  auto model = std::make_unique<Model>();

  for (auto& meshDesc : modelDesc.meshes) {
    auto submodel = std::make_unique<Submodel>();
    submodel->mesh = constructMesh(meshDesc, dataBuffers);
    submodel->material = constructMaterial(meshDesc.material);

    model->submodels.push_back(std::move(submodel));
  }

  return model;
}
