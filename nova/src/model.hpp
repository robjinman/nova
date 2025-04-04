#pragma once

#include "math.hpp"
#include <memory>
#include <vector>
#include <string>

using RenderItemId = long;
const RenderItemId NULL_ID = -1;

// TODO: Support meshes with different set of vertex attributes
//       Struct of arrays instead of array of structs?
struct Vertex
{
  Vec3f pos;
  Vec3f normal;
  Vec2f texCoord;
};

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

struct Material
{
  Vec4f colour = { 1, 1, 1, 1 };
  MaterialResource texture;
  MaterialResource cubeMap;
  MaterialResource normalMap;
  float_t metallicFactor = 0.f;
  float_t roughnessFactor = 0.f;
};

using MaterialPtr = std::unique_ptr<Material>;

using VertexList = std::vector<Vertex>;
using IndexList = std::vector<uint16_t>;

struct Mesh
{
  VertexList vertices;
  IndexList indices;
  bool isInstanced = false;
  size_t maxInstances = 1;
};

using MeshPtr = std::unique_ptr<Mesh>;

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

std::ostream& operator<<(std::ostream& stream, const Vertex& vertex);
