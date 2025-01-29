#include "math.hpp"
#include "utils.hpp"
#include <cassert>

Line::Line(const Vec2f& A, const Vec2f& B)
{
  a = B[1] - A[1];
  b = A[0] - B[0];
  c = B[0] * A[1] - A[0] * B[1];
}

Line::Line(float_t a, float_t b, float_t c)
  : a(a)
  , b(b)
  , c(c)
{
}

bool lineIntersect(const Line& l1, const Line& l2, Vec2f& p)
{
  if (l1.a * l2.b - l1.b * l2.a != 0) {
    p = {
      (l1.b * l2.c - l1.c * l2.b) / (l1.a * l2.b - l1.b * l2.a),
      (l1.c * l2.a - l1.a * l2.c) / (l1.a * l2.b - l1.b * l2.a)
    };
    return true;
  }
  return false;
}

Vec2f projectionOntoLine(const Line& line, const Vec2f& p)
{
  float_t a = -line.b;
  float_t b = line.a;
  float_t c = -p[1] * b - p[0] * a;
  Line perpendicular(a, b, c);

  Vec2f result;
  [[maybe_unused]] bool intersect = lineIntersect(line, perpendicular, result);
  assert(intersect);

  return result;
}

bool lineSegmentCircleIntersect(const LineSegment& lseg, const Vec2f& centre, float_t radius)
{
  Vec2f D = lseg.B - lseg.A;
  float_t alpha = D[0] * D[0] + D[1] * D[1];
  float_t beta = 2.0 * (D[0] * (lseg.A[0] - centre[0]) + D[1] * (lseg.A[1] - centre[1]));
  float_t gamma = pow(lseg.A[0] - centre[0], 2) + pow(lseg.A[1] - centre[1], 2) - radius * radius;

  float_t discriminant = beta * beta - 4.0 * alpha * gamma;
  if (discriminant < 0) {
    return false;
  }

  float_t t = (-beta + sqrt(discriminant)) / (2.0 * alpha);
  if (t >= 0 && t <= 1.0) {
    return true;
  }

  t = (-beta - sqrt(discriminant)) / (2.0 * alpha);
  if (t >= 0 && t <= 1.0) {
    return true;
  }

  return false;
}

bool pointIsInsidePoly(const Vec2f& p, const std::vector<Vec2f>& poly)
{
  bool inside = false;
  int n = static_cast<int>(poly.size());

  for (int i = 0; i < n; ++i) {
    float_t x1 = poly[i][0];
    float_t y1 = poly[i][1];

    float_t x2 = poly[(i + 1) % n][0];
    float_t y2 = poly[(i + 1) % n][1];

    bool intersects = ((y1 > p[1]) != (y2 > p[1]));
    if (intersects) {
      float_t xIntersect = x1 + (p[1] - y1) * (x2 - x1) / (y2 - y1);

      if (xIntersect > p[0]) {
        inside = !inside;
      }
    }
  }

  return inside;
}
