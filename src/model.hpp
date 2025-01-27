#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <string>

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 colour;
  glm::vec2 texCoord;
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
  glm::mat4 transform = glm::mat4(1);
};

using ModelPtr = std::unique_ptr<Model>;

ModelPtr loadModel(const std::string& objFilePath);
