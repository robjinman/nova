#include "map_parser.hpp"
#include "logger.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include "xml.hpp"

namespace {

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
    Mat4x4f parseMatrixTransform(const std::string& s, float_t scale) const;
    Mat4x4f parseTranslateTransform(const std::string& s, float_t scale) const;
    Mat4x4f parseTransform(const std::string& s, float_t scale) const;
    void extractGeometry(const XmlNode& node, Path& path, Mat4x4f& groupTransform,
      Mat4x4f& pathTransform, float_t scale) const;
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

  Mat4x4f groupTransform;
  Mat4x4f pathTransform;

  extractGeometry(node, obj.path, groupTransform, pathTransform, scale);
  obj.transform = groupTransform;

  for (auto& p : obj.path.points) {
    p = pathTransform * p;
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
  Vec4f pathEnd{};
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
      Vec4f p{ 0, 0, 0, 1 };

      if (component == -1) {
        char comma = '_';
        std::stringstream stream(token);
        stream >> p[0];
        stream >> comma;
        ASSERT(comma == ',', "Expected a comma");
        stream >> p[2];
      }
      else {
        std::stringstream stream(token);
        stream >> p[component];
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

  auto m = scaleMatrix<float_t, 4>(scale);

  for (auto& p : path.points) {
    p = m * p;
  }

  return path;
}

Mat4x4f MapParserImpl::parseTranslateTransform(const std::string& s, float_t scale) const
{
  std::stringstream stream(s);

  std::string buf(10, '\0');
  stream.read(&buf[0], 10);
  ASSERT(buf == "translate(", "Syntax error");

  Mat4x4f m = identityMatrix<float_t, 4>();
  char comma;
  float_t value;
  stream >> value >> comma;
  m.set(0, 3, value * scale);
  ASSERT(comma == ',', "Syntax error");
  stream >> value >> buf;
  m.set(2, 3, value * scale);
  ASSERT(buf == ")", "Syntax error");

  return m;
}

Mat4x4f MapParserImpl::parseMatrixTransform(const std::string& s, float_t scale) const
{
  DBG_LOG(m_logger, STR("Parsing SVG matrix transform: " << s));

  std::stringstream stream(s);

  std::string buf(7, '\0');
  stream.read(&buf[0], 7);
  ASSERT(buf == "matrix(", "Syntax error");

  float_t a = 0.f;
  Vec3f translation{};

  char comma;
  float_t value;
  stream >> value >> comma;                     // cos(a)
  a = acos(value);
  ASSERT(comma == ',', "Syntax error");
  stream >> value >> comma;                     // sin(a)
  ASSERT(fabs(value - a) <= 0.001, "Inconsistent rotation matrix");
  ASSERT(comma == ',', "Syntax error");
  stream >> value >> comma;                     // -sin(a)
  ASSERT(comma == ',', "Syntax error");
  stream >> value >> comma;                     // cos(a)
  ASSERT(comma == ',', "Syntax error");
  stream >> translation[0] >> comma;            // tx
  ASSERT(comma == ',', "Syntax error");
  stream >> translation[2] >> buf;              // tz
  ASSERT(buf == ")", "Syntax error");

  return transform(translation, Vec3f{ 0, a, 0 });
}

Mat4x4f MapParserImpl::parseTransform(const std::string& s, float_t scale) const
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

void MapParserImpl::extractGeometry(const XmlNode& node, Path& path, Mat4x4f& groupTransform,
  Mat4x4f& pathTransform, float_t scale) const
{
  groupTransform = identityMatrix<float_t, 4>();
  pathTransform = identityMatrix<float_t, 4>();

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

  Vec4f centre = (path.points[0] + path.points[1] + path.points[2]) / 3.0;
  Vec4f mostDistantPoint = centre;

  for (int i = 0; i < 3; ++i) {
    if ((path.points[i] - centre).magnitude() > (mostDistantPoint - centre).magnitude()) {
      mostDistantPoint = path.points[i];
    }
  }

  Vec4f v = mostDistantPoint - centre;
  float_t a = (3.f * PI / 2.f) - atan2(v[2], v[0]);  // Angle from vertical (negative z)

  return transform(Vec3f{centre[0], 0, centre[2]}, Vec3f{0, a, 0});
}

MapParserPtr createMapParser(Logger& logger)
{
  return std::make_unique<MapParserImpl>(logger);
}
