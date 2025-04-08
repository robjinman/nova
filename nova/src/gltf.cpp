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

BufferDesc extractBuffer(const nlohmann::json& root, unsigned long accessorIndex,
  ElementType elementType)
{
  auto& accessors = root.at("accessors");
  auto& accessor = accessors[accessorIndex];
  auto& bufferViews = root.at("bufferViews");

  auto numElements = accessor.at("count").get<unsigned long>();
  auto bufferViewIndex = accessor.at("bufferView").get<unsigned long>();
  auto type = accessor.at("type").get<std::string>();
  auto componentType = accessor.at("componentType").get<unsigned long>();

  auto& bufferView = bufferViews[bufferViewIndex];
  auto bufferIndex = bufferView.at("buffer").get<unsigned long>();

  auto byteLength = bufferView.at("byteLength").get<unsigned long>();
  auto byteOffset = bufferView.at("byteOffset").get<unsigned long>();

  return BufferDesc{
    .type = elementType,
    .dimensions = dimensions(type),
    .componentType = static_cast<ComponentType>(componentType),
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
  std::vector<MeshDesc>& meshDescs)
{
  auto& nodes = root.at("nodes");
  auto& node = nodes[nodeIndex];

  // TODO: Use these
  auto iRotation = node.find("rotation");
  auto iScale = node.find("scale");
  auto iTranslation = node.find("translation");

  auto iMeshIndex = node.find("mesh");
  if (iMeshIndex != node.end()) {
    auto& meshes = root.at("meshes");
    auto& mesh = meshes[iMeshIndex->get<unsigned long>()];
    auto& meshPrimitives = mesh.at("primitives");

    for (auto& meshPrimitive : meshPrimitives) {
      MeshDesc meshDesc;

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

      meshDescs.push_back(meshDesc);
    }
  }

  auto iSkin = node.find("skin");
  if (iSkin != node.end()) {
    auto skinIndex = iSkin->get<unsigned long>();
    ASSERT(skinIndex == 0, "Currently, only models with a single skin are supported");
  }

  auto iChildren = node.find("children");
  if (iChildren != node.end()) {
    auto& children = *iChildren;
    for (auto& child : children) {
      auto index = child.get<unsigned long>();
      extractMeshHierarchy(root, index, meshDescs);
    }
  }
}

JointDesc extractJointHierarchy(const nlohmann::json& nodes, unsigned long nodeIndex)
{
  auto& node = nodes[nodeIndex];

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
      rotation[0],
      rotation[1],
      rotation[2],
      rotation[3]
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

  JointDesc jointDesc;
  jointDesc.nodeIndex = nodeIndex;
  jointDesc.transform = T * R * S;

  auto iChildren = node.find("children");
  if (iChildren != node.end()) {
    auto& children = *iChildren;
    for (auto index : children) {
      jointDesc.children.push_back(extractJointHierarchy(nodes, index));
    }
  }

  return jointDesc;
}

std::vector<AnimationDesc> extractAnimations(const nlohmann::json& root)
{
  std::vector<AnimationDesc> animationDescs;

  // TODO

  return animationDescs;
}

ArmatureDesc extractArmature(const nlohmann::json& root, unsigned long rootNodeIndex)
{
  auto& nodes = root.at("nodes");

  ArmatureDesc armatureDesc;
  armatureDesc.root = extractJointHierarchy(nodes, rootNodeIndex);

  auto iSkins = root.find("skins");
  if (iSkins != root.end()) {
    auto& skins = *iSkins;
    ASSERT(skins.size() == 1, "Currently, only models with a single skin are supported");

    auto& skin = skins[0];

    auto inverseBindMatricesIndex = skin.at("inverseBindMatrices").get<unsigned long>();
    armatureDesc.skin.inverseBindMatricesBuffer = extractBuffer(root, inverseBindMatricesIndex,
      ElementType::JointInvertedBindMatrices);

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

  extractMeshHierarchy(root, rootNodeIndex, modelDesc.meshes);
  modelDesc.armature = extractArmature(root, rootNodeIndex);

  return modelDesc;
}

} // namespace gltf
