#include "model.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include <fstream>
#include <regex>

bool startsWith(const std::string& str, const std::string& prefix)
{
  return str.size() >= prefix.size() && strncmp(str.c_str(), prefix.c_str(), prefix.size()) == 0;
}

ModelPtr loadModel(const std::string& objFilePath)
{
  std::ifstream stream(objFilePath);
  std::string line;

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> uvCoords;

  std::regex vertexPattern{"v\\s+(-?\\d+\\.?\\d+)\\s+(-?\\d+\\.?\\d+)\\s+(-?\\d+\\.?\\d+)"};
  std::regex uvCoordPattern{"vt\\s+(-?\\d+\\.?\\d+)\\s+(-?\\d+\\.?\\d+)"};
  std::regex facePattern{"f\\s+(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)"};

  std::smatch match;

  auto model = std::make_unique<Model>();

  glm::vec3 colour{ 1, 1, 1 };

  while (!stream.eof()) {
    std::getline(stream, line);

    if (startsWith(line, "vt")) {
      std::regex_search(line, match, uvCoordPattern);

      ASSERT(match.size() == 3, STR("Error parsing uv coords: " << line));
      float u = std::stof(match[1].str());
      float v = std::stof(match[2].str());
  
      uvCoords.push_back({ u, v });
    }
    else if (startsWith(line, "v")) {
      std::regex_search(line, match, vertexPattern);

      ASSERT(match.size() == 4, STR("Error parsing vertex: " << line));
      float x = std::stof(match[1].str());
      float y = std::stof(match[2].str());
      float z = std::stof(match[3].str());
  
      vertices.push_back({ x, y, z });
    }
    else if (startsWith(line, "f")) {
      std::regex_search(line, match, facePattern);

      ASSERT(match.size() == 7, STR("Error parsing face: " << line));

      auto makeVertex = [&](unsigned i, unsigned j) {
        int vertexIdx = std::stoi(match[i].str()) - 1;
        int uvCoordIdx = std::stoi(match[j].str()) - 1;

        ASSERT(inRange(vertexIdx, 0, static_cast<int>(vertices.size()) - 1), "Index out of range");
        ASSERT(inRange(uvCoordIdx, 0, static_cast<int>(uvCoords.size()) - 1), "Index out of range");

        model->vertices.push_back(Vertex{ vertices[vertexIdx], colour, uvCoords[uvCoordIdx] });
        model->indices.push_back(model->vertices.size() - 1);
      };

      makeVertex(1, 2);
      makeVertex(3, 4);
      makeVertex(5, 6);
    }
  }

  // TODO: Re-index model

  return model;
}
