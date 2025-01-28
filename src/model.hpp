#pragma once

#include "math.hpp"
#include <memory>
#include <vector>
#include <string>

struct Vertex
{
  Vec3f pos;
  Vec3f colour;
  Vec2f texCoord;
};

using TextureId = size_t;

struct Texture
{
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t channels = 3;
  std::vector<uint8_t> data;
};

using TexturePtr = std::unique_ptr<Texture>;

using ModelId = size_t;

using VertexList = std::vector<Vertex>;
using IndexList = std::vector<uint16_t>;

struct Model
{
  VertexList vertices;
  IndexList indices;
  TextureId texture = 0;
  Mat4x4f transform = Mat4x4f(1);
};

using ModelPtr = std::unique_ptr<Model>;

ModelPtr loadModel(const std::string& objFilePath);
TexturePtr loadTexture(const std::string& filePath);
