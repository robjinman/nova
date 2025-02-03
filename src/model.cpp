#include "model.hpp"
#include "exception.hpp"
#include "utils.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fstream>
#include <regex>

bool startsWith(const std::string& str, const std::string& prefix)
{
  return str.size() >= prefix.size() && strncmp(str.c_str(), prefix.c_str(), prefix.size()) == 0;
}

MeshPtr loadMesh(const std::string& objFilePath)
{
  std::ifstream stream(objFilePath);
  std::string line;

  std::vector<Vec3f> vertices;
  std::vector<Vec2f> uvCoords;

  std::regex vertexPattern{"v\\s+(-?\\d+\\.?\\d+)\\s+(-?\\d+\\.?\\d+)\\s+(-?\\d+\\.?\\d+)"};
  std::regex uvCoordPattern{"vt\\s+(-?\\d+\\.?\\d+)\\s+(-?\\d+\\.?\\d+)"};
  std::regex facePattern{"f\\s+(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)"};

  std::smatch match;

  auto mesh = std::make_unique<Mesh>();

  Vec3f colour{1, 1, 1};

  while (!stream.eof()) {
    std::getline(stream, line);

    if (startsWith(line, "vt")) {
      std::regex_search(line, match, uvCoordPattern);

      ASSERT(match.size() == 3, STR("Error parsing uv coords: " << line));
      float_t u = parseFloat<float_t>(match[1].str());
      float_t v = parseFloat<float_t>(match[2].str());
  
      uvCoords.push_back({u, v});
    }
    else if (startsWith(line, "v")) {
      std::regex_search(line, match, vertexPattern);

      ASSERT(match.size() == 4, STR("Error parsing vertex: " << line));
      float_t x = parseFloat<float_t>(match[1].str());
      float_t y = parseFloat<float_t>(match[2].str());
      float_t z = parseFloat<float_t>(match[3].str());
  
      vertices.push_back({x, y, z});
    }
    else if (startsWith(line, "f")) {
      std::regex_search(line, match, facePattern);

      ASSERT(match.size() == 7, STR("Error parsing face: " << line));

      auto makeVertex = [&](unsigned i, unsigned j) {
        int vertexIdx = std::stoi(match[i].str()) - 1;
        int uvCoordIdx = std::stoi(match[j].str()) - 1;

        ASSERT(inRange(vertexIdx, 0, static_cast<int>(vertices.size()) - 1), "Index out of range");
        ASSERT(inRange(uvCoordIdx, 0, static_cast<int>(uvCoords.size()) - 1), "Index out of range");

        mesh->vertices.push_back(Vertex{ vertices[vertexIdx], colour, uvCoords[uvCoordIdx] });
        mesh->indices.push_back(mesh->vertices.size() - 1);
      };

      makeVertex(1, 2);
      makeVertex(3, 4);
      makeVertex(5, 6);
    }
  }

  // TODO: Re-index model

  return mesh;
}

TexturePtr loadTexture(const std::string& filePath)
{
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

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
