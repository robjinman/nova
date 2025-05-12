#include "renderables.hpp"
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

std::ostream& operator<<(std::ostream& stream, const render::MeshFeatureSet& features)
{
  stream << features.vertexLayout << std::endl;
  stream << features.flags;

  return stream;
}

std::ostream& operator<<(std::ostream& stream, const render::MaterialFeatureSet& features)
{
  stream << features.flags;

  return stream;
}

namespace render
{

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
    },
    .flags{}
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

  uint16_t n = static_cast<uint16_t>(A.attributeBuffers[0].numElements());
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

} // namespace render
