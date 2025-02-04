#include "map_parser.hpp"
#include "logger.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include "xml.hpp"

namespace {

Mat4x4f transform4x4(const Mat3x3f& A)
{
  return Mat4x4f{
    A.at(0, 0), A.at(0, 1), 0, A.at(0, 2),
    A.at(1, 0), A.at(1, 1), 0, A.at(1, 2),
    0, 0, 1, 0,
    0, 0, 0, 1
  };
};

bool isTriangle(const Path& path)
{
  return path.points.size() == 3 && path.closed;
}

class MapParserImpl : public MapParser
{
  public:
    MapParserImpl(Logger& logger);

    ObjectData parseMapFile(const std::string& path) const override;

  private:
    Logger& m_logger;

    ObjectData constructObjectData(const XmlNode& node, float_t scale) const;
    Path constructPath(const XmlNode& node, float_t scale) const;
    Mat3x3f parseMatrixTransform(const std::string& s, float_t scale) const;
    Mat3x3f parseTranslateTransform(const std::string& s, float_t scale) const;
    Mat3x3f parseTransform(const std::string& s, float_t scale) const;
    void extractGeometry(const XmlNode& node, Path& path, Mat3x3f& groupTransform,
      Mat3x3f& pathTransform, float_t scale) const;
    KeyValueMap parseKeyValuePairs(const XmlNode& node) const;
};

MapParserImpl::MapParserImpl(Logger& logger)
  : m_logger(logger)
{
}

ObjectData MapParserImpl::parseMapFile(const std::string& path) const
{
  XmlNodePtr root = openXmlFile(path);

  ASSERT(root->name() == "svg", "Expected root node 'svg'");

  auto& g = *root->child("g");

  try {
    auto& text = *g.child("text");
    auto& tspan = *text.child("tspan");
    float_t scale = std::stod(tspan.contents());

    for (auto& child : g) {
      if (child.name() == "g") {
        return constructObjectData(child, scale);
      }
    }
  }
  catch (const std::exception& e) {
    EXCEPTION(STR("Error parsing map file; " << e.what()));
  }

  EXCEPTION("Map contains no objects");
}

std::pair<std::string, std::string> parseKvpString(const std::string& kvp)
{
  std::stringstream stream(kvp);
  std::string key, val;

  std::getline(stream, key, '=');
  std::getline(stream, val, '=');

  return std::make_pair(key, val);
}

KeyValueMap MapParserImpl::parseKeyValuePairs(const XmlNode& node) const
{
  KeyValueMap kvpMap;

  for (auto& child : node) {
    if (child.name() == "text") {
      std::string kvpString = child.child("tspan")->contents();
      kvpMap.insert(parseKvpString(kvpString));
    }
  }

  return kvpMap;
}

ObjectData MapParserImpl::constructObjectData(const XmlNode& node, float_t scale) const
{
  ObjectData obj;
  obj.values = parseKeyValuePairs(node);
  ASSERT(obj.values.count("type") != 0, "Object is missing 'type' value");
  obj.name = obj.values.at("type");

  DBG_LOG(m_logger, STR("Parsing object of type: " << obj.name));

  Mat3x3f groupTransform;
  Mat3x3f pathTransform;

  extractGeometry(node, obj.path, groupTransform, pathTransform, scale);
  obj.transform = transform4x4(groupTransform);

  for (auto& p : obj.path.points) {
    auto q = pathTransform * Vec3f{ p[0], p[1], 1 };
    p[0] = q[0];
    p[1] = q[1];
  }

  for (auto& child : node) {
    if (child.name() == "g") {
      obj.children.push_back(constructObjectData(child, scale));
    }
  }

  return obj;
}

Path parseSvgPathString(const std::string& svgPath)
{
  Path path;
  Vec2f pathEnd{};
  std::stringstream stream(svgPath);
  bool relative = false;
  int component = -1; // 0 = x only, 1 = y only, -1 = both

  while (!stream.eof()) {
    std::string token;
    stream >> token;

    if (token.length() == 1) {
      if (token == "m" || token == "l") {
        relative = true;
        component = -1;
      }
      else if (token == "M" || token == "L") {
        relative = false;
        component = -1;
      }
      else if (token == "z" || token == "Z") {
        path.closed = true;
        ASSERT(stream.eof(), "Expected end of SVG path string");
        break;
      }
      else if (token == "v") {
        relative = true;
        component = 1;
      }
      else if (token == "V") {
        relative = false;
        component = 1;
      }
      else if (token == "h") {
        relative = true;
        component = 0;
      }
      else if (token == "H") {
        relative = false;
        component = 0;
      }
      else {
        EXCEPTION("Unknown SVG path operator '" << token << "'");
      }
    }
    else {
      Vec2f p{};

      if (component == -1) {
        char comma = '_';
        std::stringstream ss(token);
        ss >> p[0];
        ss >> comma;
        ASSERT(comma == ',', "Expected a comma");
        ss >> p[1];
      }
      else {
        std::stringstream ss(token);
        ss >> p[component];
      }

      path.points.push_back(relative ? (p + pathEnd) : p);
      pathEnd = path.points.back();
    }
  }

  return path;
}

Path MapParserImpl::constructPath(const XmlNode& node, float_t scale) const
{
  std::string svgPathString = node.attribute("d");
  Path path = parseSvgPathString(svgPathString);

  Mat2x2f scaleTransform{
    scale, 0,
    0, scale
  };

  for (auto& p : path.points) {
    p = scaleTransform * p;
  }

  return path;
}

Mat3x3f MapParserImpl::parseTranslateTransform(const std::string& s, float_t scale) const
{
  std::stringstream ss(s);

  std::string buf(10, '\0');
  ss.read(&buf[0], 10);
  ASSERT(buf == "translate(", "Syntax error");

  Mat3x3f m = identityMatrix<float_t, 3>();
  char comma;
  float_t value;
  ss >> value >> comma;
  m.set(0, 2, value * scale);
  ASSERT(comma == ',', "Syntax error");
  ss >> value >> buf;
  m.set(1, 2, value * scale);
  ASSERT(buf == ")", "Syntax error");

  return m;
}

Mat3x3f MapParserImpl::parseMatrixTransform(const std::string& s, float_t scale) const
{
  DBG_LOG(m_logger, STR("Parsing SVG matrix transform: " << s));

  std::stringstream ss(s);

  std::string buf(7, '\0');
  ss.read(&buf[0], 7);
  ASSERT(buf == "matrix(", "Syntax error");

  Mat3x3f m = identityMatrix<float_t, 3>();
  char comma;
  float_t value;
  ss >> value >> comma;
  m.set(0, 0, value);                       // cos(a)
  ASSERT(comma == ',', "Syntax error");
  ss >> value >> comma;
  m.set(1, 0, value);                       // sin(a)
  ASSERT(comma == ',', "Syntax error");
  ss >> value >> comma;
  m.set(0, 1, value);                       // -sin(a)
  ASSERT(comma == ',', "Syntax error");
  ss >> value >> comma;
  m.set(1, 1, value);                       // cos(a)
  ASSERT(comma == ',', "Syntax error");
  ss >> value >> comma;
  m.set(0, 2, value * scale);               // tx
  ASSERT(comma == ',', "Syntax error");
  ss >> value >> buf;
  m.set(1, 2, value * scale);               // ty
  ASSERT(buf == ")", "Syntax error");

  return m;
}

Mat3x3f MapParserImpl::parseTransform(const std::string& s, float_t scale) const
{
  ASSERT(s.length() > 0, "Expected non-empty string");

  if (s[0] == 't') {
    return parseTranslateTransform(s, scale);
  }
  else if (s[0] == 'm') {
    return parseMatrixTransform(s, scale);
  }

  EXCEPTION("Error parsing unknown transform '" << s << "'");
}

void MapParserImpl::extractGeometry(const XmlNode& node, Path& path, Mat3x3f& groupTransform,
  Mat3x3f& pathTransform, float_t scale) const
{
  groupTransform = identityMatrix<float_t, 3>();
  pathTransform = identityMatrix<float_t, 3>();

  std::string trans = node.attribute("transform");
  if (!trans.empty()) {
    groupTransform = parseTransform(trans, scale);
  }

  for (auto& child : node) {
    if (child.name() == "path") {
      path = constructPath(child, scale);

      trans = child.attribute("transform");
      if (!trans.empty()) {
        pathTransform = parseTransform(trans, scale);
      }

      break;
    }
  }
}

} // namespace

Mat4x4f transformFromTriangle(const Path& path)
{
  ASSERT(isTriangle(path), "Path is not a triangle");

  Vec2f centre = (path.points[0] + path.points[1] + path.points[2]) / 3.0;
  Vec2f mostDistantPoint = centre;

  for (int i = 0; i < 3; ++i) {
    if ((path.points[i] - centre).magnitude() > (mostDistantPoint - centre).magnitude()) {
      mostDistantPoint = path.points[i];
    }
  }

  Vec2f v = mostDistantPoint - centre;
  float_t a = atan2(v[1], v[0]) - PI / 2;  // Angle from vertical

  return transform(Vec3f{centre[0], centre[1], 0}, Vec3f{0, 0, a});
}

MapParserPtr createMapParser(Logger& logger)
{
  return std::make_unique<MapParserImpl>(logger);
}
