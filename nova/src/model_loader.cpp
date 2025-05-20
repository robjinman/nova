#include "model_loader.hpp"
#include "gltf.hpp"
#include "file_system.hpp"
#include "utils.hpp"
#include <set>

using render::Buffer;
using render::BufferUsage;
using render::VertexLayout;
using render::Mesh;
using render::MaterialPtr;
using render::MaterialHandle;
namespace MeshFeatures = render::MeshFeatures;
using render::MeshFeatureSet;
namespace MaterialFeatures = render::MaterialFeatures;
using render::MaterialFeatureSet;

namespace
{

template<typename T>
T convert(const char* value, gltf::ComponentType dataType)
{
  switch (dataType) {
    case gltf::ComponentType::SignedByte:
      return static_cast<T>(*reinterpret_cast<const int8_t*>(value));
    case gltf::ComponentType::UnsignedByte:
      return static_cast<T>(*reinterpret_cast<const uint8_t*>(value));
    case gltf::ComponentType::SignedShort:
      return static_cast<T>(*reinterpret_cast<const int16_t*>(value));
    case gltf::ComponentType::UnsignedShort:
      return static_cast<T>(*reinterpret_cast<const uint16_t*>(value));
    case gltf::ComponentType::UnsignedInt:
      return static_cast<T>(*reinterpret_cast<const uint32_t*>(value));
    case gltf::ComponentType::Float:
      return static_cast<T>(*reinterpret_cast<const float*>(value));
    default: EXCEPTION("Cannot convert data type");
  }
}

template<typename T>
size_t convert(const char* src, gltf::ComponentType srcType, uint32_t n, char* dest)
{
  for (uint32_t i = 0; i < n; ++i) {
    *(reinterpret_cast<T*>(dest) + i) = convert<T>(src + i * getSize(srcType), srcType);
  }
  return sizeof(T) * n;
}

// Map gltf types to engine types
size_t convert(const char* src, gltf::ElementType elementType, gltf::ComponentType srcType,
  uint32_t n, char* dest)
{
  switch (elementType) {
    case gltf::ElementType::AttrJointIndices:
      return convert<uint8_t>(src, srcType, n, dest);
    case gltf::ElementType::VertexIndex:
      return convert<uint16_t>(src, srcType, n, dest);
    case gltf::ElementType::AttrPosition:
    case gltf::ElementType::AttrNormal:
    case gltf::ElementType::AttrTexCoord:
    case gltf::ElementType::AttrJointWeights:
    case gltf::ElementType::AnimationTimestamps:
    case gltf::ElementType::JointRotation:
    case gltf::ElementType::JointScale:
    case gltf::ElementType::JointTranslation:
    case gltf::ElementType::JointInverseBindMatrices:
      return convert<float_t>(src, srcType, n, dest);
    default:
      EXCEPTION("Cannot convert element type");
  }
}

void copyToBuffer(const std::vector<std::vector<char>>& srcBuffers, char* dstBuffer,
  const gltf::BufferDesc desc)
{
  const char* src = srcBuffers[desc.index].data() + desc.offset;

  size_t srcElemSize = gltf::getSize(desc.componentType) * desc.dimensions;
  DBG_ASSERT(srcElemSize * desc.size == desc.byteLength, "Buffer has unexpected length");

  char* dstPtr = dstBuffer;
  for (unsigned long i = 0; i < desc.size; ++i) {
    dstPtr += convert(src + i * srcElemSize, desc.type, desc.componentType, desc.dimensions,
      dstPtr);
  }
}

BufferUsage getUsage(gltf::ElementType type)
{
  switch (type) {
    case gltf::ElementType::AttrPosition: return BufferUsage::AttrPosition;
    case gltf::ElementType::AttrNormal: return BufferUsage::AttrNormal;
    case gltf::ElementType::AttrTexCoord: return BufferUsage::AttrTexCoord;
    case gltf::ElementType::AttrJointIndices: return BufferUsage::AttrJointIndices;
    case gltf::ElementType::AttrJointWeights: return BufferUsage::AttrJointWeights;
    // TODO
    default: EXCEPTION("Error converting ElementType to BufferUsage");
  }
}

VertexLayout getVertexLayout(const gltf::MeshDesc& meshDesc, bool hasTangents)
{
  VertexLayout layout{};

  size_t i = 0;
  for (auto& b : meshDesc.buffers) {
    if (gltf::isAttribute(b.type)) {
      layout[i++] = getUsage(b.type);
    }
  }
  if (hasTangents) {
    layout[i] = BufferUsage::AttrTangent;
  }

  std::sort(layout.begin(), layout.end(), [](auto a, auto b) {
    // Use arithmetic underflow to make 0 (BuffeUsage::None) the largest value
    return static_cast<uint8_t>(static_cast<uint8_t>(a) - 1) <
      static_cast<uint8_t>(static_cast<uint8_t>(b) - 1);
  });

  return layout;
}

void computeMeshTangents(Mesh& mesh)
{
  auto getBuffer = [](const std::vector<Buffer>& buffers, BufferUsage usage) -> const Buffer& {
    auto i = std::find_if(buffers.begin(), buffers.end(), [usage](const Buffer& buffer) {
      return buffer.usage == usage;
    });
    DBG_ASSERT(i != buffers.end(), "Mesh does not contain buffer of that type");
    return *i;
  };

  auto& posBuffer = getBuffer(mesh.attributeBuffers, BufferUsage::AttrPosition);
  auto& uvBuffer = getBuffer(mesh.attributeBuffers, BufferUsage::AttrTexCoord);

  auto positions = render::getConstBufferData<Vec3f>(posBuffer);
  auto texCoords = render::getConstBufferData<Vec2f>(uvBuffer);
  auto indices = getConstBufferData<uint16_t>(mesh.indexBuffer);

  DBG_ASSERT(positions.size() == texCoords.size(), "Expected equal number of positions and UVs");
  DBG_ASSERT(indices.size() % 3 == 0, "Expected indices buffer size to be multiple of 3");

  std::vector<Vec3f> tangents(positions.size());

  size_t n = indices.size();
  for (size_t i = 0; i < n; i += 3) {
    uint16_t aIdx = indices[i];
    uint16_t bIdx = indices[i + 1];
    uint16_t cIdx = indices[i + 2];

    auto& posA = positions[aIdx];
    auto& posB = positions[bIdx];
    auto& posC = positions[cIdx];

    auto& uvA = texCoords[aIdx];
    auto& uvB = texCoords[bIdx];
    auto& uvC = texCoords[cIdx];

    // TODO: Simplify. Don't need the bitangent

    Mat2x2f M = inverse(Mat2x2f{
      uvB[0] - uvA[0], uvC[0] - uvB[0],
      uvB[1] - uvA[1], uvC[1] - uvB[1]
    });

    Vec3f E = posB - posA;
    Vec3f F = posC - posB;

    Mat3x2f EF{
      E[0], F[0],
      E[1], F[1],
      E[2], F[2]
    };

    Mat3x2f TB = EF * M;

    Vec3f T{ TB.at(0, 0), TB.at(1, 0), TB.at(2, 0) };

    tangents[aIdx] += T;
    tangents[bIdx] += T;
    tangents[cIdx] += T;
  }

  mesh.attributeBuffers.push_back(createBuffer(tangents, BufferUsage::AttrTangent));
}

MeshFeatureSet createMeshFeatureSet(const gltf::MeshDesc& meshDesc)
{
  auto hasAttribute = [&](gltf::ElementType type) {
    for (auto& buf : meshDesc.buffers) {
      if (buf.type == type) {
        return true;
      }
    }
    return false;
  };

  bool hasTangents = !meshDesc.material.normalMap.empty();
  bool isAnimated = hasAttribute(gltf::ElementType::AttrJointIndices);

  MeshFeatures::Flags flags{};
  flags.set(MeshFeatures::CastsShadow, true);
  flags.set(MeshFeatures::HasTangents, hasTangents);
  flags.set(MeshFeatures::IsAnimated, isAnimated);

  MeshFeatureSet features{
    .vertexLayout = getVertexLayout(meshDesc, hasTangents),
    .flags = flags
  };

  return features;
}

MaterialFeatureSet createMaterialFeatureSet(const gltf::MaterialDesc& materialDesc)
{
  MaterialFeatureSet features;

  features.flags.set(MaterialFeatures::HasTexture, !materialDesc.baseColourTexture.empty());
  features.flags.set(MaterialFeatures::HasNormalMap, !materialDesc.normalMap.empty());
  features.flags.set(MaterialFeatures::IsDoubleSided, materialDesc.isDoubleSided);

  return features;
}

render::MeshPtr constructMesh(const gltf::MeshDesc& meshDesc,
  const std::vector<std::vector<char>>& dataBuffers)
{
  auto mesh = std::make_unique<Mesh>(createMeshFeatureSet(meshDesc));
  mesh->transform = meshDesc.transform;

  // TODO: Set mesh rest pose

  std::set<gltf::ElementType> attributes;
  for (const auto& bufferDesc : meshDesc.buffers) {
    if (gltf::isAttribute(bufferDesc.type)) {
      DBG_ASSERT(!attributes.contains(bufferDesc.type),
        "Model contains multiple attribute buffers of same type");

      attributes.insert(bufferDesc.type);
    }
  }

  mesh->attributeBuffers.resize(attributes.size());

  for (const auto& bufferDesc : meshDesc.buffers) {
    if (bufferDesc.type == gltf::ElementType::VertexIndex) {
      mesh->indexBuffer.usage = BufferUsage::Index;
      mesh->indexBuffer.data.resize(bufferDesc.byteLength);
      copyToBuffer(dataBuffers, mesh->indexBuffer.data.data(), bufferDesc);
    }
    else if (gltf::isAttribute(bufferDesc.type)) {
      auto usage = getUsage(bufferDesc.type);
      Buffer buffer;
      buffer.usage = usage;
      buffer.data.resize(bufferDesc.byteLength);

      copyToBuffer(dataBuffers, buffer.data.data(), bufferDesc);

      size_t index = std::distance(attributes.begin(), attributes.find(bufferDesc.type));
      mesh->attributeBuffers[index] = std::move(buffer);
    }
    else {
      // TODO
      EXCEPTION("Not implemented");
    }
  }

  return mesh;
}

render::MaterialPtr constructMaterial(const gltf::MaterialDesc& materialDesc)
{
  auto material = std::make_unique<render::Material>(createMaterialFeatureSet(materialDesc));

  material->name = materialDesc.name;
  material->texture.fileName = materialDesc.baseColourTexture;
  material->normalMap.fileName = materialDesc.normalMap;
  material->colour = materialDesc.baseColourFactor;
  // TODO: PBR attributes

  return material;
}

class ModelLoaderImpl : public ModelLoader
{
  public:
    ModelLoaderImpl(RenderSystem& renderSystem, const FileSystem& fileSystem, Logger& logger);

    ModelDataPtr loadModelData(const std::string& filePath) const override;
    CRenderModelPtr createRenderComponent(ModelDataPtr modelData, bool isInstanced) override;

  private:
    Logger& m_logger;
    RenderSystem& m_renderSystem;
    const FileSystem& m_fileSystem;
    std::map<std::string, RenderItemId> m_materials;

    MaterialHandle loadMaterial(MaterialPtr material);
};

ModelLoaderImpl::ModelLoaderImpl(RenderSystem& renderSystem, const FileSystem& fileSystem,
  Logger& logger)
  : m_logger(logger)
  , m_renderSystem(renderSystem)
  , m_fileSystem(fileSystem)
{
}

MaterialHandle ModelLoaderImpl::loadMaterial(MaterialPtr material)
{
  auto textureFileName = material->texture.fileName;
  if (textureFileName != "") {
    auto texturePath = STR("resources/textures/" << textureFileName);

    auto i = m_materials.find(textureFileName);
    if (i == m_materials.end()) {
      auto texture = render::loadTexture(m_fileSystem.readFile(texturePath));
      material->texture.id = m_renderSystem.addTexture(std::move(texture));
      m_materials[textureFileName] = material->texture.id;
    }
    else {
      material->texture.id = i->second;
    }
  }

  // TODO: Remove duplication
  auto normalMapFileName = material->normalMap.fileName;
  if (normalMapFileName != "") {
    auto texturePath = STR("resources/textures/" << normalMapFileName);

    auto i = m_materials.find(normalMapFileName);
    if (i == m_materials.end()) {
      auto texture = render::loadTexture(m_fileSystem.readFile(texturePath));
      material->normalMap.id = m_renderSystem.addNormalMap(std::move(texture));
      m_materials[normalMapFileName] = material->normalMap.id;
    }
    else {
      material->normalMap.id = i->second;
    }
  }
  // TODO: Repeat the above for cube maps

  return m_renderSystem.addMaterial(std::move(material));
}

SkeletonPtr extractSkeleton(const gltf::ArmatureDesc& armature)
{
  auto skeleton = std::make_unique<Skeleton>();
  skeleton->rootNodeIndex = armature.rootNodeIndex;

  for (auto& node : armature.nodes) {
    skeleton->joints.push_back(Joint{
      .transform = node.transform,
      .children = node.children
    });
  }

  return skeleton;
}

SkinPtr constructSkin(const std::vector<std::vector<char>>& dataBuffers,
  const gltf::SkinDesc& skinDesc)
{
  auto skin = std::make_unique<Skin>();

  skin->inverseBindMatrices.resize(skinDesc.inverseBindMatricesBuffer.size);
  copyToBuffer(dataBuffers, reinterpret_cast<char*>(skin->inverseBindMatrices.data()),
    skinDesc.inverseBindMatricesBuffer);

  for (auto nodeNumber : skinDesc.nodeIndices) {
    skin->joints.push_back(nodeNumber);
  }

  return skin;
}

std::vector<Transform> constructJointTransformsBuffer(const std::vector<float_t>& data,
  gltf::ElementType elementType)
{
  std::vector<Transform> buffer;

  for (size_t i = 0; i < data.size();) {
    Transform transform;

    switch (elementType) {
      case gltf::ElementType::JointRotation: {
        transform.rotation = { data[i + 3], data[i + 0], data[i + 1], data[i + 2] };
        i += 4;
        break;
      }
      case gltf::ElementType::JointScale:
        transform.scale = { data[i + 0], data[i + 1], data[i + 2] };
        i += 3;
        break;
      case gltf::ElementType::JointTranslation:
        transform.translation = { data[i + 0], data[i + 1], data[i + 2] };
        i += 3;
        break;
      default:
        EXCEPTION("Unexpected element type");
    }

    buffer.push_back(transform);
  }

  return buffer;
}

ModelDataPtr ModelLoaderImpl::loadModelData(const std::string& filePath) const
{
  auto modelDesc = gltf::extractModel(m_fileSystem.readFile(filePath));

  std::vector<std::vector<char>> dataBuffers;
  for (const auto& buffer : modelDesc.buffers) {
    auto binPath = std::filesystem::path{filePath}.parent_path() / buffer;
    dataBuffers.push_back(m_fileSystem.readFile(binPath));
  }

  bool hasAnimations = modelDesc.armature.animations.size() > 0;
  auto model = std::make_unique<ModelData>();
  if (hasAnimations) {
    model->animations = std::make_unique<AnimationSet>();
    model->animations->skeleton = extractSkeleton(modelDesc.armature);
  }

  for (auto& meshDesc : modelDesc.meshes) {
    auto submodel = std::make_unique<SubmodelData>();
    submodel->mesh = constructMesh(meshDesc, dataBuffers);
    submodel->material = constructMaterial(meshDesc.material);
    if (hasAnimations) {
      submodel->skin = constructSkin(dataBuffers, meshDesc.skin);
    }

    if (submodel->mesh->featureSet.flags.test(MeshFeatures::HasTangents)) {
      computeMeshTangents(*submodel->mesh);
    }

    model->submodels.push_back(std::move(submodel));
  }

  for (auto& animationDesc : modelDesc.armature.animations) {
    std::map<size_t, std::vector<float_t>> buffers;

    auto getBuffer = [&](size_t index) -> std::vector<float_t>& {
      auto i = buffers.find(index);
      if (i != buffers.end()) {
        return i->second;
      }
      auto& bufferDesc = animationDesc.buffers[index];
      DBG_ASSERT(bufferDesc.componentType == gltf::ComponentType::Float, "Expected float buffer");
      std::vector<float_t> buffer(bufferDesc.size * bufferDesc.dimensions);
      copyToBuffer(dataBuffers, reinterpret_cast<char*>(buffer.data()), bufferDesc);
      buffers[index] = std::move(buffer);
      return buffers.at(index);
    };

    auto animation = std::make_unique<Animation>();
    animation->name = animationDesc.name;

    for (auto& channelDesc : animationDesc.channels) {
      auto& transformBufferDesc = animationDesc.buffers[channelDesc.transformsBufferIndex];
      auto& transformsBuffer = getBuffer(channelDesc.transformsBufferIndex);

      std::vector<Transform> transforms = constructJointTransformsBuffer(transformsBuffer,
        transformBufferDesc.type);

      animation->channels.push_back(AnimationChannel{
        .jointIndex = channelDesc.nodeIndex,
        .timestamps = getBuffer(channelDesc.timesBufferIndex),
        .transforms = std::move(transforms)
      });
    }

    model->animations->animations[animation->name] = std::move(animation);
  }

  return model;
}

CRenderModelPtr ModelLoaderImpl::createRenderComponent(ModelDataPtr modelData, bool isInstanced)
{
  auto id = System::nextId();
  CRenderModelPtr model = std::make_unique<CRenderModel>(id);
  model->isInstanced = isInstanced;

  for (auto& submodelData : modelData->submodels) {
    m_renderSystem.compileShader(submodelData->mesh->featureSet,
      submodelData->material->featureSet);

    Submodel submodel{
      .mesh = m_renderSystem.addMesh(std::move(submodelData->mesh)),
      .material = loadMaterial(std::move(submodelData->material)),
      .skin = std::move(submodelData->skin)
    };

    model->submodels.push_back(std::move(submodel));
  }

  model->animations = m_renderSystem.addAnimations(std::move(modelData->animations));

  return model;
}

} // namespace

ModelLoaderPtr createModelLoader(RenderSystem& renderSystem, const FileSystem& fileSystem,
  Logger& logger)
{
  return std::make_unique<ModelLoaderImpl>(renderSystem, fileSystem, logger);
}
