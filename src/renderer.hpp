#pragma once

#include "model.hpp"

class Camera;

class Renderer
{
  public:
    virtual TextureId addTexture(TexturePtr texture) = 0;
    virtual MaterialId addMaterial(MaterialPtr material) = 0;
    virtual MeshId addMesh(MeshPtr mesh) = 0;

    virtual void removeTexture(TextureId id) = 0;
    virtual void removeMaterial(MaterialId id) = 0;
    virtual void removeMesh(MeshId id) = 0;

    virtual void beginFrame(const Camera& camera) = 0;
    virtual void stageModel(MeshId mesh, MaterialId material, const Mat4x4f& transform) = 0;
    virtual void stageInstance(MeshId mesh, MaterialId material, const Mat4x4f& transform) = 0;
    virtual void endFrame() = 0;

    virtual ~Renderer() {}
};

using RendererPtr = std::unique_ptr<Renderer>;

class GLFWwindow;
class Logger;

RendererPtr createRenderer(GLFWwindow& window, Logger& logger);
