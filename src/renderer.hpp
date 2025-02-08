#pragma once

#include "model.hpp"
#include <array>

class Camera;

class Renderer
{
  public:
    virtual RenderItemId addTexture(TexturePtr texture) = 0;
    virtual RenderItemId addCubeMap(const std::array<TexturePtr, 6>& textures) = 0;
    virtual RenderItemId addMaterial(MaterialPtr material) = 0;
    virtual RenderItemId addMesh(MeshPtr mesh) = 0;

    virtual void removeTexture(RenderItemId id) = 0;
    virtual void removeCubeMap(RenderItemId id) = 0;
    virtual void removeMaterial(RenderItemId id) = 0;
    virtual void removeMesh(RenderItemId id) = 0;

    virtual void beginFrame(const Camera& camera) = 0;
    virtual void stageModel(RenderItemId mesh, RenderItemId material, const Mat4x4f& transform) = 0;
    virtual void stageInstance(RenderItemId mesh, RenderItemId material,
      const Mat4x4f& transform) = 0;
    virtual void stageSkybox(RenderItemId mesh, RenderItemId cubeMap) = 0;
    virtual void endFrame() = 0;

    virtual ~Renderer() {}
};

using RendererPtr = std::unique_ptr<Renderer>;

class GLFWwindow;
class Logger;

RendererPtr createRenderer(GLFWwindow& window, Logger& logger);
