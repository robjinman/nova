#pragma once

#include "model.hpp"
#include <array>

class Camera;

struct ViewParams
{
  float_t hFov;
  float_t vFov;
  float_t aspectRatio;
  float_t nearPlane;
  float_t farPlane;
};

class Renderer
{
  public:
    virtual void start() = 0;
    virtual double frameRate() const = 0;
    virtual void onResize() = 0;
    virtual const ViewParams& getViewParams() const = 0;
    virtual void checkError() const = 0;

    // Initialisation
    //
    virtual void compileShader(const MeshFeatureSet& meshFeatures,
      const MaterialFeatureSet& materialFeatures) = 0;

    // Resources
    //
    virtual RenderItemId addTexture(TexturePtr texture) = 0;
    virtual RenderItemId addCubeMap(std::array<TexturePtr, 6>&& textures) = 0;

    virtual void removeTexture(RenderItemId id) = 0;
    virtual void removeCubeMap(RenderItemId id) = 0;

    // Meshes
    //
    virtual RenderItemId addMesh(MeshPtr mesh) = 0;
    virtual void removeMesh(RenderItemId id) = 0;

    // Materials
    //
    virtual RenderItemId addMaterial(MaterialPtr material) = 0;
    virtual void removeMaterial(RenderItemId id) = 0;

    // Per frame draw functions
    //
    virtual void beginFrame(const Camera& camera) = 0;
    virtual void drawModel(RenderItemId mesh, RenderItemId material, const Mat4x4f& transform) = 0;
    virtual void drawInstance(RenderItemId mesh, RenderItemId material,
      const Mat4x4f& transform) = 0;
    virtual void drawLight(const Vec3f& colour, float_t ambient, const Vec3f& worldPos) = 0;
    virtual void drawSkybox(RenderItemId mesh, RenderItemId cubeMap) = 0;
    virtual void endFrame() = 0;

    virtual ~Renderer() {}
};

using RendererPtr = std::unique_ptr<Renderer>;

class FileSystem;
class WindowDelegate;
class Logger;

RendererPtr createRenderer(const FileSystem& fileSystem, WindowDelegate& window, Logger& logger);
