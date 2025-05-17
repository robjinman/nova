#pragma once

#include "math.hpp"
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
  AttrPosition,
  AttrNormal,
  AttrTexCoord,
  AttrJointIndices,
  AttrJointWeights,

  VertexIndex,

  AnimationTimestamps,
  JointInverseBindMatrices,
  JointTranslation,
  JointRotation,
  JointScale
};

inline bool isAttribute(ElementType type)
{
  switch (type) {
    case ElementType::AttrPosition:
    case ElementType::AttrNormal:
    case ElementType::AttrTexCoord:
    case ElementType::AttrJointIndices:
    case ElementType::AttrJointWeights:
      return true;
    default:
      return false;
  }
}

struct BufferDesc
{
  ElementType type = ElementType::AttrPosition;
  uint32_t dimensions = 0;
  ComponentType componentType = ComponentType::Float;
  size_t size = 0;
  size_t byteLength = 0;
  size_t offset = 0;
  size_t index = 0;
};

struct MaterialDesc
{
  std::string name;
  std::string baseColourTexture;
  std::string normalMap;
  Vec4f baseColourFactor = { 1, 1, 1, 1 };
  float metallicFactor = 0.f;
  float roughnessFactor = 0.f;
  bool isDoubleSided = false;
};

struct SkinDesc
{
  std::vector<unsigned long> nodeIndices;
  BufferDesc inverseBindMatricesBuffer;
};

struct MeshDesc
{
  std::vector<BufferDesc> buffers;
  MaterialDesc material;
  Mat4x4f transform;
  SkinDesc skin;
};

struct NodeDesc
{
  Mat4x4f transform;
  std::vector<size_t> children;
};

enum class Interpolation
{
  Step,
  Linear
};

struct AnimationChannelDesc
{
  size_t nodeIndex;
  size_t timesBufferIndex;
  size_t transformsBufferIndex;
  Interpolation interpolation;
};

struct AnimationDesc
{
  std::string name;
  std::vector<AnimationChannelDesc> channels;
  std::vector<BufferDesc> buffers;
};

struct ArmatureDesc
{
  size_t rootNodeIndex = 0;
  std::vector<NodeDesc> nodes;
  std::vector<AnimationDesc> animations;
};

struct ModelDesc
{
  std::vector<MeshDesc> meshes;
  ArmatureDesc armature;
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
    default: EXCEPTION("Cannot get size of unknown type");
  }
}

ModelDesc extractModel(const std::vector<char>& jsonData);

} // namespace gltf
