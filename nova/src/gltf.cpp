#include "gltf.hpp"
#include "exception.hpp"
#include <nlohmann/json.hpp>

namespace gltf
{

namespace
{

ElementType parseElementType(const std::string& type)
{
  if (type == "POSITION") return ElementType::AttrPosition;
  else if (type == "NORMAL") return ElementType::AttrNormal;
  else if (type == "TEXCOORD_0") return ElementType::AttrTexCoord;
  else if (type == "INDEX") return ElementType::VertexIndex;
  else if (type == "JOINTS_0") return ElementType::AttrJointIndices;
  else if (type == "WEIGHTS_0") return ElementType::AttrJointWeights;
  else EXCEPTION("Unknown attribute type '" << type << "'");
}

uint32_t dimensions(const std::string& type)
{
  if (type == "SCALAR") return 1;
  else if (type == "VEC2") return 2;
  else if (type == "VEC3") return 3;
  else if (type == "VEC4") return 4;
  else if (type == "MAT4") return 16;
  else EXCEPTION("Unknown element type '" << type << "'");
}

Mat4x4f extractTransform(const nlohmann::json& node)
{
  auto iRotation = node.find("rotation");
  auto iScale = node.find("scale");
  auto iTranslation = node.find("translation");

  Mat4x4f S = identityMatrix<float_t, 4>();
  Mat4x4f R = identityMatrix<float_t, 4>();
  Mat4x4f T = identityMatrix<float_t, 4>();

  if (iScale != node.end()) {
    auto& scale = *iScale;
    S.set(0, 0, scale[0]);
    S.set(1, 1, scale[1]);
    S.set(2, 2, scale[2]);
  }

  if (iRotation != node.end()) {
    auto& rotation = *iRotation;
    R = rotationMatrix4x4(Vec4f{
      rotation[3],
      rotation[0],
      rotation[1],
      rotation[2]
    });
  }

  if (iTranslation != node.end()) {
    auto& translation = *iTranslation;
    T = translationMatrix4x4(Vec3f{
      translation[0],
      translation[1],
      translation[2]
    });
  }

  return T * R * S;
}

BufferDesc extractBuffer(const nlohmann::json& root, unsigned long accessorIndex,
  ElementType elementType)
{
  auto& accessors = root.at("accessors");
  auto& accessor = accessors[accessorIndex];
  auto& bufferViews = root.at("bufferViews");

  auto numElements = accessor.at("count").get<unsigned long>();
  auto bufferViewIndex = accessor.at("bufferView").get<unsigned long>();
  auto type = accessor.at("type").get<std::string>();
  auto componentType =
    static_cast<ComponentType>(accessor.at("componentType").get<unsigned long>());

  auto& bufferView = bufferViews[bufferViewIndex];
  auto bufferIndex = bufferView.at("buffer").get<unsigned long>();

  auto byteLength = bufferView.at("byteLength").get<unsigned long>();
  auto byteOffset = bufferView.at("byteOffset").get<unsigned long>();

  uint32_t numDimensions = dimensions(type);

  ASSERT(byteLength == numElements * numDimensions * getSize(componentType),
    "Buffer has unexpeced length");

  return BufferDesc{
    .type = elementType,
    .dimensions = numDimensions,
    .componentType = componentType,
    .size = numElements,
    .byteLength = byteLength,
    .offset = byteOffset,
    .index = bufferIndex
  };
}

MaterialDesc extractMaterial(const nlohmann::json& root, unsigned long materialIndex)
{
  MaterialDesc materialDesc;

  auto& materials = root.at("materials");
  auto iTextures = root.find("textures");
  auto iImages = root.find("images");
  auto& material = materials[materialIndex];
  materialDesc.isDoubleSided = material.at("doubleSided").get<bool>();
  materialDesc.name = material.at("name").get<std::string>();
  auto& pbr = material.at("pbrMetallicRoughness");
  auto iBaseColourFactor = pbr.find("baseColorFactor");
  if (iBaseColourFactor != pbr.end()) {
    materialDesc.baseColourFactor[0] = (*iBaseColourFactor)[0].get<float>();
    materialDesc.baseColourFactor[1] = (*iBaseColourFactor)[1].get<float>();
    materialDesc.baseColourFactor[2] = (*iBaseColourFactor)[2].get<float>();
    materialDesc.baseColourFactor[3] = (*iBaseColourFactor)[3].get<float>();
  }
  auto iBaseColourTextureInfo = pbr.find("baseColorTexture");
  if (iBaseColourTextureInfo != pbr.end()) {
    auto baseColourTextureIndex = iBaseColourTextureInfo->at("index").get<unsigned long>();
    auto& baseColourTexture = (*iTextures)[baseColourTextureIndex];
    auto baseColourSourceIndex = baseColourTexture.at("source").get<unsigned long>();
    auto& baseColourImage = (*iImages)[baseColourSourceIndex];
    materialDesc.baseColourTexture = baseColourImage.at("uri").get<std::string>();
  }
  auto iNormalTextureInfo = material.find("normalTexture");
  if (iNormalTextureInfo != material.end()) {
    auto normalTextureIndex = iNormalTextureInfo->at("index").get<unsigned long>();
    auto& normalTexture = (*iTextures)[normalTextureIndex];
    auto normalSourceIndex = normalTexture.at("source").get<unsigned long>();
    auto& normalImage = (*iImages)[normalSourceIndex];
    materialDesc.normalMap = normalImage.at("uri").get<std::string>();
  }

  // TODO: Don't assume these are present
  //materialDesc.metallicFactor = pbr.at("metallicFactor").get<float>();
  //materialDesc.roughnessFactor = pbr.at("roughnessFactor").get<float>();

  return materialDesc;
}

void extractMeshHierarchy(const nlohmann::json& root, unsigned long nodeIndex,
  std::vector<MeshDesc>& meshDescs, const Mat4x4f& parentTransform)
{
  auto& nodes = root.at("nodes");
  auto& node = nodes[nodeIndex];

  Mat4x4f localTransform = extractTransform(node);
  Mat4x4f globalTransform = parentTransform * localTransform;

  auto iMeshIndex = node.find("mesh");
  if (iMeshIndex != node.end()) {
    auto& meshes = root.at("meshes");
    auto& mesh = meshes[iMeshIndex->get<unsigned long>()];
    auto& meshPrimitives = mesh.at("primitives");

    for (auto& meshPrimitive : meshPrimitives) {
      MeshDesc meshDesc;
      meshDesc.transform = globalTransform;

      auto& meshAttributes = meshPrimitive.at("attributes");

      for (const auto& [ attributeName, accessorIndex ] : meshAttributes.items()) {
        auto index = accessorIndex.get<unsigned long>();
        auto bufferDesc = extractBuffer(root, index, parseElementType(attributeName));
        meshDesc.buffers.push_back(bufferDesc);
      }

      auto indexBufferIndex = meshPrimitive.at("indices").get<unsigned long>();
      meshDesc.buffers.push_back(extractBuffer(root, indexBufferIndex, ElementType::VertexIndex));

      auto materialIndex = meshPrimitive.at("material").get<unsigned long>();
      meshDesc.material = extractMaterial(root, materialIndex);

      auto iSkin = node.find("skin");
      if (iSkin != node.end()) {
        auto skinIndex = iSkin->get<unsigned long>();
        auto& skins = root.at("skins");
        auto& skin = skins[skinIndex];

        auto inverseBindMatricesIndex = skin.at("inverseBindMatrices").get<unsigned long>();
        meshDesc.skin = SkinDesc{
          .nodeIndices = skin.at("joints").get<std::vector<unsigned long>>(),
          .inverseBindMatricesBuffer = extractBuffer(root, inverseBindMatricesIndex,
            ElementType::JointInverseBindMatrices)
        };
      }

      meshDescs.push_back(meshDesc);
    }
  }

  auto iChildren = node.find("children");
  if (iChildren != node.end()) {
    auto& children = *iChildren;
    for (auto& child : children) {
      auto index = child.get<unsigned long>();
      extractMeshHierarchy(root, index, meshDescs, globalTransform);
    }
  }
}

std::vector<NodeDesc> extractNodeHierarchy(const nlohmann::json& root)
{
  std::vector<NodeDesc> nodeDescs;
  auto& nodes = root.at("nodes");

  for (auto& node : nodes) {
    NodeDesc nodeDesc;
    nodeDesc.transform = extractTransform(node);

    auto iChildren = node.find("children");
    if (iChildren != node.end()) {
      auto& children = *iChildren;
      for (auto index : children) {
        nodeDesc.children.push_back(index);
      }
    }

    nodeDescs.push_back(nodeDesc);
  }

  return nodeDescs;
}

Interpolation parseInterpolation(const std::string& s)
{
  if (s == "STEP") {
    return Interpolation::Step;
  }
  else if (s == "LINEAR") {
    return Interpolation::Linear;
  }
  else {
    EXCEPTION("Error parsing interpolation '" << s << "'");
  }
}

std::vector<AnimationDesc> extractAnimations(const nlohmann::json& root)
{
  std::vector<AnimationDesc> animationDescs;

  auto& animations = root.at("animations");

  // Map accessor index to position in animation's buffers array
  std::map<size_t, size_t> bufferIndices;

  auto elementTypeFromString = [](const std::string& s) {
    if (s == "translation") {
      return ElementType::JointTranslation;
    }
    else if (s == "rotation") {
      return ElementType::JointRotation;
    }
    else if (s == "scale") {
      return ElementType::JointScale;
    }
    EXCEPTION("Unrecognised joint transform type");
  };

  for (auto& animation : animations) {
    AnimationDesc animDesc;
    animDesc.name = animation.at("name").get<std::string>();

    auto& channels = animation.at("channels");
    auto& samplers = animation.at("samplers");

    for (auto& channel : channels) {
      AnimationChannelDesc channelDesc;
      auto& target = channel.at("target");
      auto samplerIndex = channel.at("sampler").get<unsigned long>();
      auto& sampler = samplers[samplerIndex];
      auto timesAccessorIndex = sampler.at("input").get<unsigned long>();
      auto transformsAccessorIndex = sampler.at("output").get<unsigned long>();
      auto transformType = elementTypeFromString(target.at("path").get<std::string>());

      if (!bufferIndices.contains(timesAccessorIndex)) {
        size_t i = animDesc.buffers.size();
        animDesc.buffers.push_back(extractBuffer(root, timesAccessorIndex,
          ElementType::AnimationTimestamps));
        bufferIndices[timesAccessorIndex] = i;
      }

      if (!bufferIndices.contains(transformsAccessorIndex)) {
        size_t i = animDesc.buffers.size();
        animDesc.buffers.push_back(extractBuffer(root, transformsAccessorIndex, transformType));
        bufferIndices[transformsAccessorIndex] = i;
      }

      channelDesc.timesBufferIndex = bufferIndices.at(timesAccessorIndex);
      channelDesc.transformsBufferIndex = bufferIndices.at(transformsAccessorIndex);
      channelDesc.nodeIndex = target.at("node").get<unsigned long>();
      channelDesc.interpolation = parseInterpolation(sampler.at("interpolation"));

      animDesc.channels.push_back(std::move(channelDesc));
    }

    animationDescs.push_back(std::move(animDesc));
  }

  return animationDescs;
}

ArmatureDesc extractArmature(const nlohmann::json& root, unsigned long rootNodeIndex)
{
  ArmatureDesc armatureDesc;
  armatureDesc.rootNodeIndex = rootNodeIndex;
  armatureDesc.nodes = extractNodeHierarchy(root);

  if (root.contains("animations")) {
    armatureDesc.animations = extractAnimations(root);
  }

  return armatureDesc;
}

} // namespace

ModelDesc extractModel(const std::vector<char>& jsonData)
{
  auto root = nlohmann::json::parse(jsonData);
  auto& scenes = root.at("scenes");
  auto sceneIndex = root.at("scene").get<unsigned long>();
  auto& scene = scenes[sceneIndex];
  auto& sceneNodes = scene.at("nodes");
  // TODO
  ASSERT(sceneNodes.size() == 1,
    "Expected scene to contain 1 root node, found " << sceneNodes.size());
  auto rootNodeIndex = sceneNodes[0].get<unsigned long>();

  ModelDesc modelDesc;

  auto& buffers = root.at("buffers");
  for (auto& buffer : buffers) {
    modelDesc.buffers.push_back(buffer.at("uri").get<std::string>());
  }

  extractMeshHierarchy(root, rootNodeIndex, modelDesc.meshes, identityMatrix<float_t, 4>());
  modelDesc.armature = extractArmature(root, rootNodeIndex);

  return modelDesc;
}

} // namespace gltf
