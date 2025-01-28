#pragma once

#include "model.hpp"

class Camera;

class Renderer
{
public:
  virtual void update(const Camera& camera) = 0;

  virtual TextureId addTexture(TexturePtr texture) = 0;
  virtual void removeTexture(TextureId id) = 0;

  virtual ModelId addModel(ModelPtr model) = 0;
  virtual void removeModel(ModelId id) = 0;

  virtual Mat4x4f getModelTransform(ModelId modelId) const = 0;
  virtual void setModelTransform(ModelId modelId, const Mat4x4f& transform) = 0;

  virtual ~Renderer() {}
};

using RendererPtr = std::unique_ptr<Renderer>;

class GLFWwindow;
class Logger;

RendererPtr createRenderer(GLFWwindow& window, Logger& logger);
