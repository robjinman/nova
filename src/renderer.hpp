#pragma once

#include "model.hpp"

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
