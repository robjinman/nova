#pragma once

#include "math.hpp"
#include <memory>
#include <vector>
#include <string>

using RenderItemId = long;
const RenderItemId NULL_ID = -1;

struct Vertex
{
  Vec3f pos;
  Vec3f colour;
  Vec2f texCoord;
};

struct Texture
{
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t channels = 3;
  std::vector<uint8_t> data;
};

using TextureId = RenderItemId;
using TexturePtr = std::unique_ptr<Texture>;

using VertexList = std::vector<Vertex>;
using IndexList = std::vector<uint16_t>;

struct Material
{
  TextureId texture = NULL_ID;
  TextureId normalMap = NULL_ID;
};

using MaterialId = RenderItemId;
using MaterialPtr = std::unique_ptr<Material>;

struct Mesh
{
  VertexList vertices;
  IndexList indices;
  bool isInstanced = false;
  size_t maxInstances = 1;
};

using MeshId = RenderItemId;
using MeshPtr = std::unique_ptr<Mesh>;

MeshPtr loadMesh(const std::string& objFilePath);
TexturePtr loadTexture(const std::string& filePath);
