#pragma once

#include "math.hpp"
#include "hash.hpp"
#include <memory>
#include <vector>
#include <string>
#include <span>
#include <optional>

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

enum class BufferUsage : int
{
  AttrPosition,
  AttrNormal,
  AttrTexCoord,
  AttrJointIndices,
  AttrJointWeights,
  Index
};

inline size_t getAttributeSize(BufferUsage usage)
{
  switch (usage) {
    case BufferUsage::AttrPosition: return sizeof(Vec3f);
    case BufferUsage::AttrNormal: return sizeof(Vec3f);
    case BufferUsage::AttrTexCoord: return sizeof(Vec2f);
    case BufferUsage::AttrJointIndices: return sizeof(uint8_t) * 4;
    case BufferUsage::AttrJointWeights: return sizeof(float_t) * 4;
    case BufferUsage::Index: return sizeof(uint16_t);
  }
  EXCEPTION("Error getting element size");
}

// TODO: Make these more compact with bitfields
struct MeshFeatureSet
{
  std::vector<BufferUsage> vertexLayout;
  bool isInstanced = false;
  bool isSkybox = false;
  bool isAnimated = false;
  uint32_t maxInstances = 0;

  bool operator==(const MeshFeatureSet& rhs) const = default;
};

struct MaterialFeatureSet
{
  bool hasTransparency = false;
  bool hasTexture = false;
  bool hasNormalMap = false;

  bool operator==(const MaterialFeatureSet& rhs) const = default;
};

struct MeshHandle
{
  RenderItemId id = NULL_ID;
  MeshFeatureSet features;
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
      x.isInstanced,
      x.isSkybox,
      x.isAnimated,
      x.maxInstances
    );
  }
};

template<>
struct std::hash<MaterialFeatureSet>
{
  size_t operator()(const MaterialFeatureSet& x) const noexcept
  {
    return hashAll(
      x.hasTransparency,
      x.hasTexture,
      x.hasNormalMap
    );
  }
};

struct Material
{
  Material(const MaterialFeatureSet& features)
    : featureSet(features)
  {}

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

inline size_t calcOffsetInVertex(const std::vector<BufferUsage>& layout, BufferUsage attribute)
{
  const static std::array<BufferUsage, 5> attributeOrder{ // TODO: Move this
    BufferUsage::AttrPosition,
    BufferUsage::AttrNormal,
    BufferUsage::AttrTexCoord,
    BufferUsage::AttrJointIndices,
    BufferUsage::AttrJointWeights
  };

  auto isBefore = [&](BufferUsage a, BufferUsage b) {
    if (a == b) {
      return false;
    }
    for (auto attr : attributeOrder) {
      if (attr == a) {
        return true;
      }
      else if (attr == b) {
        return false;
      }
    }
    EXCEPTION("Error calculating attribute offset");
  };

  size_t sum = 0;
  for (auto attr : layout) {
    if (isBefore(attr, attribute)) {
      sum += getAttributeSize(attr);
    }
  }
  return sum;
}

struct Mesh
{
  Mesh(const MeshFeatureSet& features)
    : featureSet(features)
  {}

  MeshFeatureSet featureSet;
  std::vector<Buffer> attributeBuffers;
  Buffer indexBuffer;
};

using MeshPtr = std::unique_ptr<Mesh>;

template<typename T>
std::span<const T> getConstAttrBufferData(const Mesh& mesh, size_t index, BufferUsage usage)
{
  auto& buffer = mesh.attributeBuffers[index];
  size_t attributeSize = getAttributeSize(buffer.usage);

  DBG_ASSERT(buffer.usage == usage, "Buffer has unexpected type");
  DBG_ASSERT(buffer.data.size() % attributeSize == 0, "Buffer has unexpected size");

  return std::span<const T>(
    reinterpret_cast<const T*>(buffer.data.data()),
    buffer.data.size() / attributeSize
  );
}

template<typename T>
std::span<T> getAttrBufferData(Mesh& mesh, size_t index, BufferUsage usage)
{
  auto span = getConstAttrBufferData<T>(mesh, index, usage);
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

enum class TransformationType
{
  Rotation,
  Translation,
  Scale
};

struct AnimationChannel
{
  size_t joint;
  TransformationType type;
  std::vector<float_t> times;
  std::vector<Vec4f> values;
};

struct Animation
{
  std::vector<AnimationChannel> channels;
};

struct Armature
{
  std::vector<Joint> joints;
  std::vector<Animation> animations;
};

using ArmaturePtr = std::unique_ptr<Armature>;

struct Model
{
  std::vector<SubmodelPtr> submodels;
  ArmaturePtr armature;
};

using ModelPtr = std::unique_ptr<Model>;

class FileSystem;
TexturePtr loadTexture(const std::vector<char>& data);
ModelPtr loadModel(const FileSystem& fileSystem, const std::string& filePath);
MeshPtr cuboid(float_t w, float_t h, float_t d, const Vec2f& textureSize);
MeshPtr mergeMeshes(const Mesh& A, const Mesh& B);
std::vector<char> createVertexArray(const Mesh& mesh);
