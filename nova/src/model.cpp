#include "model.hpp"
#include "utils.hpp"
#include "exception.hpp"
#include "file_system.hpp"
#include <nlohmann/json.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fstream>
#include <regex>
#include <cassert>
#include <iostream> // TODO

std::ostream& operator<<(std::ostream& stream, const Vertex& vertex)
{
  stream << "Vertex{ pos: (" << vertex.pos << "), normal: (" << vertex.normal << "), tex coord: ("
    << vertex.texCoord << ") }";

  return stream;
}

bool startsWith(const std::string& str, const std::string& prefix)
{
  return str.size() >= prefix.size() && strncmp(str.c_str(), prefix.c_str(), prefix.size()) == 0;
}

// TODO: This is very slow
MeshPtr loadMesh(const std::vector<char>& data)
{
  std::stringstream stream;
  stream.write(data.data(), data.size());
  stream.seekg(0, std::ios::beg);

  std::string line;

  std::vector<Vec3f> vertices;
  std::vector<Vec3f> normals;
  std::vector<Vec2f> uvCoords;

  std::regex vertexPattern{
    "v\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)"};
  std::regex normalPattern{
    "vn\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)"};
  std::regex uvCoordPattern{
    "vt\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)"};
  std::regex facePattern{
    "f\\s+(\\d+)/(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)/(\\d+)"};

  std::smatch match;

  auto mesh = std::make_unique<Mesh>();

  while (!stream.eof()) {
    std::getline(stream, line);

    if (startsWith(line, "vt")) {
      std::regex_search(line, match, uvCoordPattern);

      ASSERT(match.size() == 3, STR("Error parsing uv coords: " << line));
      float_t u = parseFloat<float_t>(match[1].str());
      float_t v = parseFloat<float_t>(match[2].str());
  
      uvCoords.push_back({ u, v });
    }
    else if (startsWith(line, "vn")) {
      std::regex_search(line, match, normalPattern);

      ASSERT(match.size() == 4, STR("Error parsing normal: " << line));
      float_t x = parseFloat<float_t>(match[1].str());
      float_t y = parseFloat<float_t>(match[2].str());
      float_t z = parseFloat<float_t>(match[3].str());
  
      normals.push_back({ x, y, z });
    }
    else if (startsWith(line, "v")) {
      std::regex_search(line, match, vertexPattern);

      ASSERT(match.size() == 4, STR("Error parsing vertex: " << line));
      float_t x = parseFloat<float_t>(match[1].str());
      float_t y = parseFloat<float_t>(match[2].str());
      float_t z = parseFloat<float_t>(match[3].str());
  
      vertices.push_back({ x, y, z });
    }
    else if (startsWith(line, "f")) {
      std::regex_search(line, match, facePattern);

      ASSERT(match.size() == 10, STR("Error parsing face: " << line));

      auto makeVertex = [&](unsigned i, unsigned j, unsigned k) {
        int vertexIdx = std::stoi(match[i].str()) - 1;
        int uvCoordIdx = std::stoi(match[j].str()) - 1;
        int normalIdx = std::stoi(match[k].str()) - 1;

        ASSERT(inRange(vertexIdx, 0, static_cast<int>(vertices.size()) - 1), "Index out of range");
        ASSERT(inRange(uvCoordIdx, 0, static_cast<int>(uvCoords.size()) - 1), "Index out of range");
        ASSERT(inRange(normalIdx, 0, static_cast<int>(normals.size()) - 1), "Index out of range");

        Vertex vertex{};
        vertex.pos = vertices[vertexIdx];
        vertex.normal = normals[normalIdx];
        vertex.texCoord = uvCoords[uvCoordIdx];

        mesh->vertices.push_back(vertex);
        mesh->indices.push_back(static_cast<uint16_t>(mesh->vertices.size()) - 1);
      };

      makeVertex(1, 2, 3);
      makeVertex(4, 5, 6);
      makeVertex(7, 8, 9);
    }
    else {
      std::cout << line << "\n";
    }
  }

  assert(mesh->indices.size() % 3 == 0);

  // TODO: Re-index model

  return mesh;
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

namespace gltf
{

enum class ComponentType : unsigned long
{
  SignedByte = 5120,
  UnsignedByte = 5121,
  SignedShort = 5122,
  UnsignedShort = 5123,
  UnsignedInt = 5125,
  Float = 5126
};

enum class ElementType
{
  Position,
  Normal,
  TexCoord,
  Index
};

struct BufferDesc
{
  ElementType type;
  uint32_t dimensions;
  ComponentType componentType;
  size_t size;
  size_t byteLength;
  size_t offset;
  size_t index;
};

uint32_t dimensions(const std::string& type)
{
  if (type == "SCALAR") return 1;
  else if (type == "VEC2") return 2;
  else if (type == "VEC3") return 3;
  else EXCEPTION("Unknown element type '" << type << "'");
}

ElementType parseElementType(const std::string& type)
{
  if (type == "POSITION") return ElementType::Position;
  else if (type == "NORMAL") return ElementType::Normal;
  else if (type == "TEXCOORD_0") return ElementType::TexCoord;
  else if (type == "INDEX") return ElementType::Index;
  else EXCEPTION("Unknown attribute type '" << type << "'");
}

size_t getSize(ComponentType type)
{
  switch (type) {
    case ComponentType::SignedByte: return 1;
    case ComponentType::UnsignedByte: return 1;
    case ComponentType::SignedShort: return 2;
    case ComponentType::UnsignedShort: return 2;
    case ComponentType::UnsignedInt: return 4;
    case ComponentType::Float: return 4;
  }
}

std::vector<BufferDesc> extractBufferDescs(const nlohmann::json& json, size_t meshIndex)
{
  auto& meshes = json.at("meshes");
  auto& accessors = json.at("accessors");
  auto& bufferViews = json.at("bufferViews");

  auto& mesh = meshes[meshIndex];

  auto& meshPrimitives = mesh.at("primitives");
  ASSERT(meshPrimitives.size() == 1, "Expect mesh to contain 1 set of primitives, but found "
    << meshPrimitives.size());
  auto& meshPrimitive = meshPrimitives[0];
  auto& meshAttributes = meshPrimitive.at("attributes");

  auto extractDesc = [&](const std::string& attributeName, unsigned long attributeIndex) {
    auto& accessor = accessors[attributeIndex];

    auto numElements = accessor.at("count").get<unsigned long>();
    auto bufferViewIndex = accessor.at("bufferView").get<unsigned long>();
    auto type = accessor.at("type").get<std::string>();
    auto componentType = accessor.at("componentType").get<unsigned long>();

    auto& bufferView = bufferViews[bufferViewIndex];
    auto bufferIndex = bufferView.at("buffer").get<unsigned long>();

    auto byteLength = bufferView.at("byteLength").get<unsigned long>();
    auto byteOffset = bufferView.at("byteOffset").get<unsigned long>();

    return BufferDesc{
      .type = parseElementType(attributeName),
      .dimensions = dimensions(type),
      .componentType = static_cast<ComponentType>(componentType),
      .size = numElements,
      .byteLength = byteLength,
      .offset = byteOffset,
      .index = bufferIndex
    };
  };

  std::vector<BufferDesc> descs;

  for (const auto& [ attributeName, attributeIndex ] : meshAttributes.items()) {
    descs.push_back(extractDesc(attributeName, attributeIndex.get<unsigned long>()));
  }

  auto indexBufferIndex = meshPrimitive.at("indices").get<unsigned long>();
  descs.push_back(extractDesc("INDEX", indexBufferIndex));

  return descs;
}

template<typename T>
T convert(const char* p, ComponentType dataType)
{
  switch (dataType) {
    case ComponentType::SignedByte: return static_cast<T>(*reinterpret_cast<const int8_t*>(p));
    case ComponentType::UnsignedByte: return static_cast<T>(*reinterpret_cast<const uint8_t*>(p));
    case ComponentType::SignedShort: return static_cast<T>(*reinterpret_cast<const int16_t*>(p));
    case ComponentType::UnsignedShort: return static_cast<T>(*reinterpret_cast<const uint16_t*>(p));
    case ComponentType::UnsignedInt: return static_cast<T>(*reinterpret_cast<const uint32_t*>(p));
    case ComponentType::Float: return static_cast<T>(*reinterpret_cast<const float*>(p));
  }
}

template<typename T>
void convert(const char* src, ComponentType srcType, uint32_t n, T* dest)
{
  for (uint32_t i = 0; i < n; ++i) {
    *(dest + i) = convert<T>(src + i * getSize(srcType), srcType);
  }
}

template<typename T>
void copyToBuffer(const std::vector<std::vector<char>>& srcBuffers, char* dstBuffer,
  size_t structSize, size_t offsetIntoStruct, const BufferDesc& desc)
{
  const char* src = srcBuffers[desc.index].data() + desc.offset;
  for (unsigned long i = 0; i < desc.size; ++i) {
    size_t srcElemSize = getSize(desc.componentType) * desc.dimensions;
    T* dstPtr = reinterpret_cast<T*>(dstBuffer + i * structSize + offsetIntoStruct);
    convert<T>(src + i * srcElemSize, desc.componentType, desc.dimensions, dstPtr);
  }
}

}

// TODO
MeshPtr loadModel(const FileSystem& fileSystem, const std::string& filePath)
{
  auto jsonData = fileSystem.readFile(filePath);

  auto root = nlohmann::json::parse(jsonData);
  auto& scenes = root.at("scenes");
  auto& nodes = root.at("nodes");
  auto sceneIndex = root.at("scene").get<unsigned long>();
  auto& scene = scenes[sceneIndex];
  auto& sceneNodes = scene.at("nodes");
  ASSERT(sceneNodes.size() == 1, "Assume scene contains 1 root node, found " << sceneNodes.size());
  auto rootNodeIndex = sceneNodes[0].get<unsigned long>();
  auto& rootNode = nodes[rootNodeIndex];
  auto meshIndex = rootNode.at("mesh").get<unsigned long>();
  auto bufferDescs = gltf::extractBufferDescs(root, meshIndex);
  auto& buffers = root.at("buffers");

  std::vector<std::vector<char>> dataBuffers;
  for (const auto& buffer : buffers) {
    auto binFileName = buffer.at("uri").get<std::string>();
    auto binPath = std::filesystem::path{filePath}.parent_path() / binFileName;

    dataBuffers.push_back(fileSystem.readFile(binPath));
  }

  MeshPtr mesh = std::make_unique<Mesh>();

  auto getOffset = [](gltf::ElementType type) {
    switch (type) {
      case gltf::ElementType::Position: return offsetof(Vertex, pos);
      case gltf::ElementType::Normal: return offsetof(Vertex, normal);
      case gltf::ElementType::TexCoord: return offsetof(Vertex, texCoord);
      default: return 0ul;
    }
  };

  for (const auto& bufferDesc : bufferDescs) {
    if (bufferDesc.type == gltf::ElementType::Index) {
      mesh->indices.resize(bufferDesc.size);
      gltf::copyToBuffer<uint16_t>(dataBuffers, reinterpret_cast<char*>(mesh->indices.data()),
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
      gltf::copyToBuffer<float_t>(dataBuffers, reinterpret_cast<char*>(mesh->vertices.data()),
        sizeof(Vertex), getOffset(bufferDesc.type), bufferDesc);
    }
  }

  return mesh;
}
