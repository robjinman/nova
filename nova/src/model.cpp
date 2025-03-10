#include "model.hpp"
#include "exception.hpp"
#include "utils.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fstream>
#include <regex>
#include <cassert>
#include <iostream> // TODO

bool startsWith(const std::string& str, const std::string& prefix)
{
  return str.size() >= prefix.size() && strncmp(str.c_str(), prefix.c_str(), prefix.size()) == 0;
}

// TODO: This is very slow
MeshPtr loadMesh(const std::vector<char>& data)
{
  std::stringstream stream;
  stream.write(data.data(), data.size());
  stream.seekg(0, std::ios::beg);

  std::string line;

  std::vector<Vec3f> vertices;
  std::vector<Vec3f> normals;
  std::vector<Vec2f> uvCoords;

  std::regex vertexPattern{
    "v\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)"};
  std::regex normalPattern{
    "vn\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)"};
  std::regex uvCoordPattern{
    "vt\\s+(-?\\d+(?:\\.\\d+)?)\\s+(-?\\d+(?:\\.\\d+)?)"};
  std::regex facePattern{
    "f\\s+(\\d+)/(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)/(\\d+)"};

  std::smatch match;

  auto mesh = std::make_unique<Mesh>();

  Vec3f colour{ 1, 1, 1 };

  while (!stream.eof()) {
    std::getline(stream, line);

    if (startsWith(line, "vt")) {
      std::regex_search(line, match, uvCoordPattern);

      ASSERT(match.size() == 3, STR("Error parsing uv coords: " << line));
      float_t u = parseFloat<float_t>(match[1].str());
      float_t v = parseFloat<float_t>(match[2].str());
  
      uvCoords.push_back({ u, v });
    }
    else if (startsWith(line, "vn")) {
      std::regex_search(line, match, normalPattern);

      ASSERT(match.size() == 4, STR("Error parsing normal: " << line));
      float_t x = parseFloat<float_t>(match[1].str());
      float_t y = parseFloat<float_t>(match[2].str());
      float_t z = parseFloat<float_t>(match[3].str());
  
      normals.push_back({ x, y, z });
    }
    else if (startsWith(line, "v")) {
      std::regex_search(line, match, vertexPattern);

      ASSERT(match.size() == 4, STR("Error parsing vertex: " << line));
      float_t x = parseFloat<float_t>(match[1].str());
      float_t y = parseFloat<float_t>(match[2].str());
      float_t z = parseFloat<float_t>(match[3].str());
  
      vertices.push_back({ x, y, z });
    }
    else if (startsWith(line, "f")) {
      std::regex_search(line, match, facePattern);

      ASSERT(match.size() == 10, STR("Error parsing face: " << line));

      auto makeVertex = [&](unsigned i, unsigned j, unsigned k) {
        int vertexIdx = std::stoi(match[i].str()) - 1;
        int uvCoordIdx = std::stoi(match[j].str()) - 1;
        int normalIdx = std::stoi(match[k].str()) - 1;

        ASSERT(inRange(vertexIdx, 0, static_cast<int>(vertices.size()) - 1), "Index out of range");
        ASSERT(inRange(uvCoordIdx, 0, static_cast<int>(uvCoords.size()) - 1), "Index out of range");
        ASSERT(inRange(normalIdx, 0, static_cast<int>(normals.size()) - 1), "Index out of range");

        Vertex vertex{};
        vertex.pos = vertices[vertexIdx];
        vertex.normal = normals[normalIdx];
        vertex.colour = colour;
        vertex.texCoord = uvCoords[uvCoordIdx];

        mesh->vertices.push_back(vertex);
        mesh->indices.push_back(static_cast<uint16_t>(mesh->vertices.size()) - 1);
      };

      makeVertex(1, 2, 3);
      makeVertex(4, 5, 6);
      makeVertex(7, 8, 9);
    }
    else {
      std::cout << line << "\n";
    }
  }

  assert(mesh->indices.size() % 3 == 0);

  // TODO: Re-index model

  return mesh;
}

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

MeshPtr cuboid(float_t W, float_t H, float_t D, const Vec3f& colour, const Vec2f& textureSize)
{
  float_t w = W / 2.f;
  float_t h = H / 2.f;
  float_t d = D / 2.f;

  float_t u = textureSize[0];
  float_t v = textureSize[1];

  MeshPtr mesh = std::make_unique<Mesh>();
  // Viewed from above
  //
  // A +---+ B
  //   |   |
  // D +---+ C
  //
  mesh->vertices = {
    // Bottom face
    {{ -w, -h, -d }, { 0, -1, 0 }, colour, { 0, 0 }},         // A  0
    {{ w, -h, -d }, { 0, -1, 0 }, colour, { W / u, 0 }},      // B  1
    {{ w, -h, d }, { 0, -1, 0 }, colour, { W /u, D / v }},    // C  2
    {{ -w, -h, d }, { 0, -1, 0 }, colour, { 0, D / v }},      // D  3

    // Top face
    {{ -w, h, d }, { 0, 1, 0 }, colour, { 0, D / v }},        // D' 4
    {{ w, h, d }, { 0, 1, 0 }, colour, { W / u, D / v }},     // C' 5
    {{ w, h, -d }, { 0, 1, 0 }, colour, { W / u, 0 }},        // B' 6
    {{ -w, h, -d }, { 0, 1, 0 }, colour, { 0, 0 }},           // A' 7

    // Right face
    {{ w, -h, d }, { 1, 0, 0 }, colour, { D / u, 0 }},        // C  8
    {{ w, -h, -d }, { 1, 0, 0 }, colour, { 0, 0 }},           // B  9
    {{ w, h, -d }, { 1, 0, 0 }, colour, { 0, H / v }},        // B' 10
    {{ w, h, d }, { 1, 0, 0 }, colour, { D / u, H / v }},     // C' 11

    // Left face
    {{ -w, -h, -d }, { -1, 0, 0 }, colour, { 0, 0 }},         // A  12
    {{ -w, -h, d }, { -1, 0, 0 }, colour, { D / u, 0 }},      // D  13
    {{ -w, h, d }, { -1, 0, 0 }, colour, { D / u, H / v }},   // D' 14
    {{ -w, h, -d }, { -1, 0, 0 }, colour, { 0, H / v }},      // A' 15

    // Far face
    {{ -w, -h, -d }, { 0, 0, -1 }, colour, { 0, 0 }},         // A  16
    {{ -w, h, -d }, { 0, 0, -1 }, colour, { 0, H / v }},      // A' 17
    {{ w, h, -d }, { 0, 0, -1 }, colour, { W / u, H / v }},   // B' 18
    {{ w, -h, -d }, { 0, 0, -1 }, colour, { W / u, 0 }},      // B  19

    // Near face
    {{ -w, -h, d }, { 0, 0, 1 }, colour, { 0, 0 }},           // D  20
    {{ w, -h, d }, { 0, 0, 1 }, colour, { W / u, 0 }},        // C  21
    {{ w, h, d }, { 0, 0, 1 }, colour, { W / u, H / v }},     // C' 22
    {{ -w, h, d }, { 0, 0, 1 }, colour, { 0, H / v }},        // D' 23
  };
  mesh->indices = {
    0, 1, 2, 0, 2, 3,         // Bottom face
    4, 5, 6, 4, 6, 7,         // Top face
    8, 9, 10, 8, 10, 11,      // Left face
    12, 13, 14, 12, 14, 15,   // Right face
    16, 17, 18, 16, 18, 19,   // Near face
    20, 21, 22, 20, 22, 23,   // Far face
  };

  return mesh;
}
