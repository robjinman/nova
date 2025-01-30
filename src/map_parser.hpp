#pragma once

#include "math.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

struct Path
{
  std::vector<Vec2f> points;
  bool closed = false;
};

using KeyValueMap = std::map<std::string, std::string>;

struct ObjectData
{
  std::string name;
  Path path;
  KeyValueMap values;
  Mat4x4f transform = identityMatrix<float_t, 4>();
  std::vector<ObjectData> children;
};

Mat4x4f transformFromTriangle(const Path& path);

class MapParser
{
  public:
    virtual ObjectData parseMapFile(const std::string& path) const = 0;

    virtual ~MapParser() {}
};

using MapParserPtr = std::unique_ptr<MapParser>;

class Logger;

MapParserPtr createMapParser(Logger& logger);
