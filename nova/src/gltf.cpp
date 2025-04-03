#include "gltf.hpp"
#include "exception.hpp"
#include <nlohmann/json.hpp>

namespace gltf
{

ElementType parseElementType(const std::string& type)
{
  if (type == "POSITION") return ElementType::Position;
  else if (type == "NORMAL") return ElementType::Normal;
  else if (type == "TEXCOORD_0") return ElementType::TexCoord;
  else if (type == "INDEX") return ElementType::Index;
  else EXCEPTION("Unknown attribute type '" << type << "'");
}

uint32_t dimensions(const std::string& type)
{
  if (type == "SCALAR") return 1;
  else if (type == "VEC2") return 2;
  else if (type == "VEC3") return 3;
  else EXCEPTION("Unknown element type '" << type << "'");
}

BufferDesc extractBufferDesc(const nlohmann::json& root, const std::string& attributeName,
  unsigned long attributeIndex)
{
  auto& accessors = root.at("accessors");
  auto& accessor = accessors[attributeIndex];
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
    .type = parseElementType(attributeName),
    .dimensions = dimensions(type),
    .componentType = static_cast<ComponentType>(componentType),
    .size = numElements,
    .byteLength = byteLength,
    .offset = byteOffset,
    .index = bufferIndex
  };
}

MaterialDesc extractMaterialDesc(const nlohmann::json& root, unsigned long materialIndex)
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
  materialDesc.metallicFactor = pbr.at("metallicFactor").get<float>();
  materialDesc.roughnessFactor = pbr.at("roughnessFactor").get<float>();

  return materialDesc;
}

ModelDesc extractModelDesc(const std::vector<char>& jsonData)
{
  auto root = nlohmann::json::parse(jsonData);
  auto& scenes = root.at("scenes");
  auto& nodes = root.at("nodes");
  auto sceneIndex = root.at("scene").get<unsigned long>();
  auto& scene = scenes[sceneIndex];
  auto& sceneNodes = scene.at("nodes");
  ASSERT(sceneNodes.size() == 1,
    "Expected scene to contain 1 root node, found " << sceneNodes.size());
  auto rootNodeIndex = sceneNodes[0].get<unsigned long>();
  auto& rootNode = nodes[rootNodeIndex];
  auto meshIndex = rootNode.at("mesh").get<unsigned long>();
  auto& buffers = root.at("buffers");
  auto& meshes = root.at("meshes");
  auto& mesh = meshes[meshIndex];
  auto& meshPrimitives = mesh.at("primitives");

  ModelDesc modelDesc;

  for (auto& buffer : buffers) {
    modelDesc.buffers.push_back(buffer.at("uri").get<std::string>());
  }

  for (auto& meshPrimitive : meshPrimitives) {
    MeshDesc meshDesc;

    auto& meshAttributes = meshPrimitive.at("attributes");

    for (const auto& [ attributeName, attributeIndex ] : meshAttributes.items()) {
      auto index = attributeIndex.get<unsigned long>();
      auto bufferDesc = extractBufferDesc(root, attributeName, index);
      meshDesc.buffers.push_back(bufferDesc);
    }

    auto indexBufferIndex = meshPrimitive.at("indices").get<unsigned long>();
    meshDesc.buffers.push_back(extractBufferDesc(root, "INDEX", indexBufferIndex));

    auto materialIndex = meshPrimitive.at("material").get<unsigned long>();
    meshDesc.material = extractMaterialDesc(root, materialIndex);

    modelDesc.meshes.push_back(meshDesc);
  }

  return modelDesc;
}

} // namespace gltf
