#pragma once

#include <vector>

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

struct MaterialDesc
{
  std::string baseColourTexture;
  std::string normalMap;
  float metallicFactor;
  float roughnessFactor;
};

struct MeshDesc
{
  std::vector<BufferDesc> buffers;
  MaterialDesc material;
};

struct ModelDesc
{
  std::vector<MeshDesc> meshes;
  std::vector<std::string> buffers;
};

inline size_t getSize(ComponentType type)
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

ModelDesc extractModelDesc(const std::vector<char>& jsonData);

} // namespace gltf
