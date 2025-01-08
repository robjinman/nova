#pragma once

#include <memory>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Renderer
{
public:
  virtual void beginFrame() = 0;
  virtual void endFrame() = 0;

  virtual ~Renderer() {}
};

using RendererPtr = std::unique_ptr<Renderer>;

class GLFWwindow;
class Logger;

RendererPtr CreateRenderer(GLFWwindow& window, Logger& logger);
