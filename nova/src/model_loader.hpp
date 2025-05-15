#pragma once

#include "render_system.hpp"
#include <map>

struct SubmodelData
{
  render::MeshPtr mesh;
  render::MaterialPtr material;
  SkinPtr skin;
};

using SubmodelDataPtr = std::unique_ptr<SubmodelData>;

struct ModelData
{
  std::vector<SubmodelDataPtr> submodels;
  AnimationSetPtr animations;
};

using ModelDataPtr = std::unique_ptr<ModelData>;

class ModelLoader
{
  public:
    virtual ModelDataPtr loadModelData(const std::string& filePath) const = 0;
    virtual CRenderModelPtr createRenderComponent(ModelDataPtr modelData, bool isInstanced) = 0;

    virtual ~ModelLoader() {}
};

using ModelLoaderPtr = std::unique_ptr<ModelLoader>;

class FileSystem;
class Logger;

ModelLoaderPtr createModelLoader(RenderSystem& renderSystem, const FileSystem& fileSystem,
  Logger& logger);
