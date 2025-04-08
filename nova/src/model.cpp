#include "model.hpp"
#include "utils.hpp"
#include "exception.hpp"
#include "file_system.hpp"
#include "gltf.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fstream>
#include <cassert>
#include <set>
#include <iostream>

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
    case gltf::ElementType::AttrPosition:
    case gltf::ElementType::AttrNormal:
    case gltf::ElementType::AttrTexCoord:
    case gltf::ElementType::AttrJointWeights:
      return convert<float_t>(src, srcType, n, dest);
    case gltf::ElementType::AttrJointIndices:
      return convert<uint8_t>(src, srcType, n, dest);
    case gltf::ElementType::VertexIndex:
      return convert<uint16_t>(src, srcType, n, dest);
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
    dstPtr += convert(src + i * srcElemSize, desc.type, desc.componentType, desc.dimensions, dstPtr);
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

std::vector<BufferUsage> getVertexLayout(const gltf::MeshDesc& meshDesc)
{
  std::vector<BufferUsage> layout;

  for (auto& b : meshDesc.buffers) {
    if (gltf::isAttribute(b.type)) {
      layout.push_back(getUsage(b.type));
    }
  }

  std::sort(layout.begin(), layout.end());

  return layout;
}

MeshFeatureSet createMeshFeatureSet(const gltf::MeshDesc& meshDesc)
{
  return MeshFeatureSet{
    .vertexLayout = getVertexLayout(meshDesc),
    .isInstanced = false, // TODO
    .isSkybox = false,
    .isAnimated = false, // TODO
    .maxInstances = 0 // TODO
  };
}

MaterialFeatureSet createMaterialFeatureSet(const gltf::MaterialDesc& materialDesc)
{
  return MaterialFeatureSet{
    .hasTransparency = false, // TODO
    .hasTexture = !materialDesc.baseColourTexture.empty(),
    .hasNormalMap = !materialDesc.normalMap.empty()
  };
}

MeshPtr constructMesh(const gltf::MeshDesc& meshDesc,
  const std::vector<std::vector<char>>& dataBuffers)
{
  auto mesh = std::make_unique<Mesh>(createMeshFeatureSet(meshDesc));

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

MaterialPtr constructMaterial(const gltf::MaterialDesc& materialDesc)
{
  auto material = std::make_unique<Material>(createMaterialFeatureSet(materialDesc));

  material->texture.fileName = materialDesc.baseColourTexture;
  material->normalMap.fileName = materialDesc.normalMap;
  material->colour = materialDesc.baseColourFactor;
  // TODO: PBR attributes

  return material;
}

} // namespace

TexturePtr loadTexture(const std::vector<char>& data)
{
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data.data()),
    static_cast<int>(data.size()), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    EXCEPTION("Failed to load texture image");
  }

  TexturePtr texture = std::make_unique<Texture>();
  texture->width = width;
  texture->height = height;
  texture->channels = channels;
  texture->data.resize(width * height * channels);
  memcpy(texture->data.data(), pixels, width * height * channels);

  stbi_image_free(pixels);

  return texture;
}

MeshPtr cuboid(float_t W, float_t H, float_t D, const Vec2f& textureSize)
{
  float_t w = W / 2.f;
  float_t h = H / 2.f;
  float_t d = D / 2.f;

  float_t u = textureSize[0];
  float_t v = textureSize[1];

  MeshPtr mesh = std::make_unique<Mesh>(MeshFeatureSet{
    .vertexLayout = {
      BufferUsage::AttrPosition,
      BufferUsage::AttrNormal,
      BufferUsage::AttrTexCoord
    }
  });
  // Viewed from above
  //
  // A +---+ B
  //   |   |
  // D +---+ C
  //
  mesh->attributeBuffers = {
    Buffer{
      .usage = BufferUsage::AttrPosition,
      .data = toBytes(std::vector<Vec3f>{
        // Bottom face
        { -w, -h, -d }, // A  0
        { w, -h, -d },  // B  1
        { w, -h, d },   // C  2
        { -w, -h, d },  // D  3

        // Top face
        { -w, h, d },   // D' 4
        { w, h, d },    // C' 5
        { w, h, -d },   // B' 6
        { -w, h, -d },  // A' 7

        // Right face
        { w, -h, d },   // C  8
        { w, -h, -d },  // B  9
        { w, h, -d },   // B' 10
        { w, h, d },    // C' 11

        // Left face
        { -w, -h, -d }, // A  12
        { -w, -h, d },  // D  13
        { -w, h, d },   // D' 14
        { -w, h, -d },  // A' 15

        // Far face
        { -w, -h, -d }, // A  16
        { -w, h, -d },  // A' 17
        { w, h, -d },   // B' 18
        { w, -h, -d },  // B  19

        // Near face
        { -w, -h, d },  // D  20
        { w, -h, d },   // C  21
        { w, h, d },    // C' 22
        { -w, h, d }    // D' 23
      })
    },
    Buffer{
      .usage = BufferUsage::AttrNormal,
      .data = toBytes(std::vector<Vec3f>{
        // Bottom face
        { 0, -1, 0 },   // A  0
        { 0, -1, 0 },   // B  1
        { 0, -1, 0 },   // C  2
        { 0, -1, 0 },   // D  3

        // Top face
        { 0, 1, 0 },    // D' 4
        { 0, 1, 0 },    // C' 5
        { 0, 1, 0 },    // B' 6
        { 0, 1, 0 },    // A' 7

        // Right face
        { 1, 0, 0 },    // C  8
        { 1, 0, 0 },    // B  9
        { 1, 0, 0 },    // B' 10
        { 1, 0, 0 },    // C' 11

        // Left face
        { -1, 0, 0 },   // A  12
        { -1, 0, 0 },   // D  13
        { -1, 0, 0 },   // D' 14
        { -1, 0, 0 },   // A' 15

        // Far face
        { 0, 0, -1 },   // A  16
        { 0, 0, -1 },   // A' 17
        { 0, 0, -1 },   // B' 18
        { 0, 0, -1 },   // B  19

        // Near face
        { 0, 0, 1 },    // D  20
        { 0, 0, 1 },    // C  21
        { 0, 0, 1 },    // C' 22
        { 0, 0, 1 }     // D' 23
      })
    },
    Buffer{
      .usage = BufferUsage::AttrTexCoord,
      .data = toBytes(std::vector<Vec2f>{
        // Bottom face
        { 0, 0 },         // A  0
        { W / u, 0 },     // B  1
        { W /u, D / v },  // C  2
        { 0, D / v },     // D  3

        // Top face
        { 0, D / v },     // D' 4
        { W / u, D / v }, // C' 5
        { W / u, 0 },     // B' 6
        { 0, 0 },         // A' 7

        // Right face
        { D / u, 0 },     // C  8
        { 0, 0 },         // B  9
        { 0, H / v },     // B' 10
        { D / u, H / v }, // C' 11

        // Left face
        { 0, 0 },         // A  12
        { D / u, 0 },     // D  13
        { D / u, H / v }, // D' 14
        { 0, H / v },     // A' 15

        // Far face
        { 0, 0 },         // A  16
        { 0, H / v },     // A' 17
        { W / u, H / v }, // B' 18
        { W / u, 0 },     // B  19

        // Near face
        { 0, 0 },         // D  20
        { W / u, 0 },     // C  21
        { W / u, H / v }, // C' 22
        { 0, H / v }      // D' 23
      })
    }
  };
  mesh->indexBuffer = Buffer{
    .usage = BufferUsage::Index,
    .data = toBytes(std::vector<uint16_t>{
      0, 1, 2, 0, 2, 3,         // Bottom face
      4, 5, 6, 4, 6, 7,         // Top face
      8, 9, 10, 8, 10, 11,      // Left face
      12, 13, 14, 12, 14, 15,   // Right face
      16, 17, 18, 16, 18, 19,   // Near face
      20, 21, 22, 20, 22, 23    // Far face
    })
  };

  return mesh;
}

ModelPtr loadModel(const FileSystem& fileSystem, const std::string& filePath)
{
  auto modelDesc = gltf::extractModel(fileSystem.readFile(filePath));

  std::vector<std::vector<char>> dataBuffers;
  for (const auto& buffer : modelDesc.buffers) {
    auto binPath = std::filesystem::path{filePath}.parent_path() / buffer;
    dataBuffers.push_back(fileSystem.readFile(binPath));
  }

  auto model = std::make_unique<Model>();

  for (auto& meshDesc : modelDesc.meshes) {
    auto submodel = std::make_unique<Submodel>();
    submodel->mesh = constructMesh(meshDesc, dataBuffers);
    submodel->material = constructMaterial(meshDesc.material);

    model->submodels.push_back(std::move(submodel));
  }

  return model;
}

MeshPtr mergeMeshes(const Mesh& A, const Mesh& B)
{
  DBG_ASSERT(A.featureSet == B.featureSet, "Cannot merge meshes with different feature sets");
  DBG_ASSERT(A.attributeBuffers.size() == B.attributeBuffers.size(),
    "Cannot merge meshes with different number of attribute buffers");

  MeshPtr mesh = std::make_unique<Mesh>(A.featureSet);

  size_t numBuffers = A.attributeBuffers.size();
  DBG_ASSERT(numBuffers > 0, "Cannot merge meshes with no attribute buffers");

  for (size_t i = 0; i < numBuffers; ++i) {
    auto& bufA = A.attributeBuffers[i];
    auto& bufB = B.attributeBuffers[i];

    DBG_ASSERT(bufA.usage == bufB.usage, "Expected equal buffer type");

    mesh->attributeBuffers.push_back(Buffer{
      .usage = bufA.usage,
      .data = {}
    });
    auto& buf = mesh->attributeBuffers.back().data;
    buf.insert(buf.end(), bufA.data.begin(), bufA.data.end());
    buf.insert(buf.end(), bufB.data.begin(), bufB.data.end());
  }

  auto indices = fromBytes<uint16_t>(A.indexBuffer.data);
  auto indicesB = fromBytes<uint16_t>(B.indexBuffer.data);

  size_t n = A.attributeBuffers[0].numElements();
  std::transform(indicesB.begin(), indicesB.end(), std::back_inserter(indices),
    [n](uint16_t i) { return i + n; });

  mesh->indexBuffer = createBuffer(indices, BufferUsage::Index);

  return mesh;
}

std::vector<char> createVertexArray(const Mesh& mesh)
{
  ASSERT(mesh.attributeBuffers.size() > 0, "Expected at least 1 attribute buffer");

  size_t numVertices = mesh.attributeBuffers[0].numElements();
  size_t vertexSize = 0;
  for (auto& buffer : mesh.attributeBuffers) {
    vertexSize += getAttributeSize(buffer.usage);

    ASSERT(buffer.numElements() == numVertices,
      "Expected all attribute buffers to have same length");
  }

  std::vector<char> array(numVertices * vertexSize);

  for (auto& buffer : mesh.attributeBuffers) {
    const char* srcPtr = buffer.data.data();
    char* destPtr = array.data();
    for (size_t i = 0; i < numVertices; ++i) {
      size_t offset = calcOffsetInVertex(mesh.featureSet.vertexLayout, buffer.usage);
      size_t attributeSize = getAttributeSize(buffer.usage);
      memcpy(destPtr + offset, srcPtr, attributeSize);

      srcPtr += attributeSize;
      destPtr += vertexSize;
    }
  }

  return array;
}
