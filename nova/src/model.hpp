#pragma once

#include "math.hpp"
#include "hash.hpp"
#include <memory>
#include <vector>
#include <string>
#include <span>
#include <optional>
#include <array>
#include <bitset>
#include <map>

using RenderItemId = long;
const RenderItemId NULL_ID = -1;

template<typename T>
std::vector<char> toBytes(const std::vector<T>& data)
{
  const char* p = reinterpret_cast<const char*>(data.data());
  size_t n = data.size() * sizeof(T);
  return std::vector<char>(p, p + n);
}

template<typename T>
std::vector<T> fromBytes(const std::vector<char>& data)
{
  const T* p = reinterpret_cast<const T*>(data.data());
  DBG_ASSERT(data.size() % sizeof(T) == 0, "Cannot convert vector");
  size_t n = data.size() / sizeof(T);
  return std::vector<T>(p, p + n);
}

struct Texture
{
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t channels = 3;
  std::vector<uint8_t> data;
};

using TexturePtr = std::unique_ptr<Texture>;

struct MaterialResource
{
  std::string fileName;
  RenderItemId id = NULL_ID;
};

enum class BufferUsage : uint8_t
{
  None = 0,
  AttrPosition = 1,
  AttrNormal,
  AttrTexCoord,
  AttrTangent,
  AttrJointIndices,
  AttrJointWeights,
  Index
};
const uint32_t LAST_ATTR_IDX = static_cast<uint32_t>(BufferUsage::AttrJointWeights);
const uint32_t MAX_ATTRIBUTES = LAST_ATTR_IDX;

inline std::ostream& operator<<(std::ostream& stream, const BufferUsage& usage)
{
  return stream << static_cast<int>(usage);
}

inline size_t getAttributeSize(BufferUsage usage)
{
  switch (usage) {
    case BufferUsage::None:             return 0;
    case BufferUsage::AttrPosition:     return sizeof(Vec3f);
    case BufferUsage::AttrNormal:       return sizeof(Vec3f);
    case BufferUsage::AttrTexCoord:     return sizeof(Vec2f);
    case BufferUsage::AttrTangent:      return sizeof(Vec3f);
    case BufferUsage::AttrJointIndices: return sizeof(uint8_t) * 4;
    case BufferUsage::AttrJointWeights: return sizeof(float_t) * 4;
    case BufferUsage::Index:            return sizeof(uint16_t);
  }
  EXCEPTION("Error getting element size");
}

namespace MeshFeatures
{
enum Enum : uint64_t
{
  IsInstanced,
  IsSkybox,
  IsAnimated,
  HasTangents,
  CastsShadow
};
using Flags = std::bitset<32>;
}

using VertexLayout = std::array<BufferUsage, MAX_ATTRIBUTES>;

struct MeshFeatureSet
{
  VertexLayout vertexLayout;
  MeshFeatures::Flags flags;

  bool operator==(const MeshFeatureSet& rhs) const = default;
};

std::ostream& operator<<(std::ostream& stream, const MeshFeatureSet& features);

namespace MaterialFeatures
{
enum Enum : uint64_t
{
  HasTransparency,
  HasTexture,
  HasNormalMap,
  HasCubeMap,
  IsDoubleSided
};
using Flags = std::bitset<32>;
}

struct MaterialFeatureSet
{
  MaterialFeatures::Flags flags;

  bool operator==(const MaterialFeatureSet& rhs) const = default;
};

std::ostream& operator<<(std::ostream& stream, const MaterialFeatureSet& features);

struct MeshHandle
{
  RenderItemId id = NULL_ID;
  MeshFeatureSet features;
  Mat4x4f transform;
};

struct MaterialHandle
{
  RenderItemId id = NULL_ID;
  MaterialFeatureSet features;
};

template<>
struct std::hash<MeshFeatureSet>
{
  size_t operator()(const MeshFeatureSet& x) const noexcept
  {
    return hashAll(
      x.vertexLayout,
      x.flags
    );
  }
};

template<>
struct std::hash<MaterialFeatureSet>
{
  size_t operator()(const MaterialFeatureSet& x) const noexcept
  {
    return hashAll(
      x.flags
    );
  }
};

struct Material
{
  Material(const MaterialFeatureSet& features)
    : featureSet(features)
  {}

  std::string name;
  MaterialFeatureSet featureSet;
  Vec4f colour = { 1, 1, 1, 1 };
  MaterialResource texture;
  MaterialResource cubeMap;
  MaterialResource normalMap;
  float_t metallicFactor = 0.f;
  float_t roughnessFactor = 0.f;
};

using MaterialPtr = std::unique_ptr<Material>;

struct Buffer
{
  BufferUsage usage;
  std::vector<char> data;

  size_t numElements() const
  {
    return data.size() / getAttributeSize(usage);
  }
};

template<typename T>
Buffer createBuffer(const std::vector<T>& data, BufferUsage usage)
{
  return Buffer{
    .usage = usage,
    .data = toBytes(data)
  };
}

inline size_t calcOffsetInVertex(const VertexLayout& layout, BufferUsage attribute)
{
  size_t sum = 0;
  for (auto attr : layout) {
    if (attr < attribute) {
      sum += getAttributeSize(attr);
    }
  }
  return sum;
}

struct Skin
{
  RenderItemId skeleton = NULL_ID;
};

using SkinPtr = std::unique_ptr<Skin>;

struct Mesh
{
  Mesh(const MeshFeatureSet& features)
    : featureSet(features)
  {}

  Mat4x4f transform = identityMatrix<float_t, 4>();
  MeshFeatureSet featureSet;
  std::vector<Buffer> attributeBuffers;
  Buffer indexBuffer;
  uint32_t maxInstances = 0;
  SkinPtr skin;
};

using MeshPtr = std::unique_ptr<Mesh>;

template<typename T>
std::span<const T> getConstBufferData(const Buffer& buffer)
{
  size_t attributeSize = getAttributeSize(buffer.usage);

  DBG_ASSERT(buffer.data.size() % attributeSize == 0, "Buffer has unexpected size");

  return std::span<const T>(
    reinterpret_cast<const T*>(buffer.data.data()),
    buffer.data.size() / attributeSize
  );
}

template<typename T>
std::span<T> getBufferData(Buffer& buffer)
{
  auto span = getConstBufferData<T>(buffer);
  return std::span<T>(const_cast<T*>(span.data()), span.size());
}

inline std::span<const uint16_t> getConstIndexBufferData(const Mesh& mesh)
{
  return std::span<const uint16_t>(
    reinterpret_cast<const uint16_t*>(mesh.indexBuffer.data.data()),
    mesh.indexBuffer.data.size() / sizeof(uint16_t)
  );
}

inline std::span<uint16_t> getIndexBufferData(Mesh& mesh)
{
  auto span = getConstIndexBufferData(mesh);
  return std::span<uint16_t>(const_cast<uint16_t*>(span.data()), span.size());
}

struct Submodel
{
  MeshPtr mesh;
  MaterialPtr material;
};

using SubmodelPtr = std::unique_ptr<Submodel>;

const size_t JOINT_MAX_CHILDREN = 8;

struct Joint
{
  Mat4x4f transform;
  std::array<uint32_t, JOINT_MAX_CHILDREN> children;
};

enum class TransformType
{
  Rotation,
  Translation,
  Scale
};

struct AnimationChannel
{
  size_t jointIndex;
  TransformType transformType;
  std::vector<float_t> timestamps;
  std::vector<Vec4f> transforms;
};

struct Animation
{
  std::vector<AnimationChannel> channels;
};

struct Model
{
  std::vector<SubmodelPtr> submodels;
  Joint skeleton;
  std::map<std::string, Animation> animations;
};

using ModelPtr = std::unique_ptr<Model>;

class FileSystem;
TexturePtr loadTexture(const std::vector<char>& data);
ModelPtr loadModel(const FileSystem& fileSystem, const std::string& filePath);
MeshPtr cuboid(float_t w, float_t h, float_t d, const Vec2f& textureSize);
MeshPtr mergeMeshes(const Mesh& A, const Mesh& B);
std::vector<char> createVertexArray(const Mesh& mesh);
