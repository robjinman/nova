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

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 colour;
  glm::vec2 texCoord;
};

using ModelId = size_t;
using VertexList = std::vector<Vertex>;
using IndexList = std::vector<uint16_t>;

struct Model
{
  VertexList vertices;
  IndexList indices;
};

using ModelPtr = std::unique_ptr<Model>;

class Renderer
{
public:
  virtual void update() = 0;
  virtual ModelId addModel(ModelPtr model) = 0;
  virtual void removeModel(ModelId id) = 0;

  virtual ~Renderer() {}
};

using RendererPtr = std::unique_ptr<Renderer>;

class GLFWwindow;
class Logger;

RendererPtr CreateRenderer(GLFWwindow& window, Logger& logger);
