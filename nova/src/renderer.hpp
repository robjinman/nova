#pragma once

#include "model.hpp"

struct ViewParams
{
  float_t hFov;
  float_t vFov;
  float_t aspectRatio;
  float_t nearPlane;
  float_t farPlane;
};

enum class RenderPass
{
  Shadow,
  Main,
  Ssr
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
    virtual RenderItemId addNormalMap(TexturePtr texture) = 0;
    virtual RenderItemId addCubeMap(std::array<TexturePtr, 6>&& textures) = 0;

    virtual void removeTexture(RenderItemId id) = 0;
    virtual void removeCubeMap(RenderItemId id) = 0;

    // Meshes
    //
    virtual MeshHandle addMesh(MeshPtr mesh) = 0;
    virtual void removeMesh(RenderItemId id) = 0;

    // Materials
    //
    virtual MaterialHandle addMaterial(MaterialPtr material) = 0;
    virtual void removeMaterial(RenderItemId id) = 0;

    // Per frame draw functions
    //
    virtual void beginFrame() = 0;
    virtual void beginPass(RenderPass renderPass, const Vec3f& viewPos,
      const Mat4x4f& viewMatrix) = 0;
    virtual void drawModel(MeshHandle mesh, MaterialHandle material, const Mat4x4f& transform) = 0;
    virtual void drawInstance(MeshHandle mesh, MaterialHandle material,
      const Mat4x4f& transform) = 0;
    virtual void drawLight(const Vec3f& colour, float_t ambient, float_t specular,
      const Mat4x4f& transform) = 0;
    virtual void drawSkybox(MeshHandle mesh, MaterialHandle cubeMap) = 0;
    virtual void endPass() = 0;
    virtual void endFrame() = 0;

    virtual ~Renderer() {}
};

using RendererPtr = std::unique_ptr<Renderer>;

class FileSystem;
class WindowDelegate;
class Logger;

RendererPtr createRenderer(const FileSystem& fileSystem, WindowDelegate& window, Logger& logger);
