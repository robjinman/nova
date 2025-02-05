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

Mat4x4f lookAt(const Vec3f& eye, const Vec3f& centre)
{
  Mat4x4f m = identityMatrix<float_t, 4>();
  Vec3f f((centre - eye).normalise());
  Vec3f s(f.cross({ 0, 1, 0 }).normalise());
  Vec3f u(s.cross(f));
  m.set(0, 0, s[0]);
  m.set(0, 1, s[1]);
  m.set(0, 2, s[2]);
  m.set(1, 0, u[0]);
  m.set(1, 1, u[1]);
  m.set(1, 2, u[2]);
  m.set(2, 0, -f[0]);
  m.set(2, 1, -f[1]);
  m.set(2, 2, -f[2]);
  m.set(0, 3, -s.dot(eye));
  m.set(1, 3, -u.dot(eye));
  m.set(2, 3, f.dot(eye));
  return m;
}

Mat4x4f perspective(float_t fovY, float_t aspect, float_t near, float_t far)
{
  Mat4x4f m;
  const float_t fovX = aspect * fovY;
  const float_t t = -near * tan(fovY * 0.5);
  const float_t b = -t;
  const float_t r = near * tan(fovX * 0.5);
  const float_t l = -r;

  m.set(0, 0, 2.0 * near / (r - l));
  m.set(0, 2, (r + l) / (r - l));
  m.set(1, 1, -2.0 * near / (b - t));
  m.set(1, 2, (b + t) / (b - t));
  m.set(2, 2, -far / (far - near));
  m.set(2, 3, -far * near / (far - near));
  m.set(3, 2, -1.0);
  m.set(3, 3, 0.0);

  return m;
}
