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
  glm::mat4 transform;
};

using ModelPtr = std::unique_ptr<Model>;

class Renderer
{
public:
  virtual void beginFrame() = 0;
  virtual void endFrame() = 0;

  virtual TextureId addTexture(TexturePtr texture) = 0;
  virtual void removeTexture(TextureId id) = 0;

  virtual ModelId addModel(ModelPtr model) = 0;
  virtual void removeModel(ModelId id) = 0;

  virtual glm::mat4 getModelTransform(ModelId modelId) const = 0;
  virtual void setModelTransform(ModelId modelId, const glm::mat4& transform) = 0;

  virtual ~Renderer() {}
};

using RendererPtr = std::unique_ptr<Renderer>;

class GLFWwindow;
class Logger;

RendererPtr CreateRenderer(GLFWwindow& window, Logger& logger);
