#pragma once

#include "exception.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <vector>

using float_t = float;

constexpr double PI = 3.14159265359;

template<typename T>
inline T degreesToRadians(T degrees)
{
  constexpr T x = static_cast<T>(PI / 180.0);
  return degrees * x;
}

template<typename T>
inline T radiansToDegrees(T radians)
{
  constexpr T x = static_cast<T>(360.0 / (2.0 * PI));
  return radians * x;
}

template<typename T> T clip(T value, T min, T max)
{
  return std::max(min, std::min(max, value));
}

template<typename T>
T sine(T a)
{
  return static_cast<T>(sin(static_cast<T>(a)));
}

template<typename T>
T cosine(T a)
{
  return static_cast<T>(cos(static_cast<T>(a)));
}

template<typename T, size_t N>
class Vector
{
  public:
    Vector() : m_data{}
    {
    }

    Vector(const std::initializer_list<T>& data)
    {
      ASSERT(data.size() == N, "Vector initialised with incorrect number of values");
      std::copy(data.begin(), data.end(), m_data.begin());
    }

    template<size_t M>
    Vector(const Vector<T, M>& rhs, const std::initializer_list<T>& rest)
    {
      static_assert(M <= N, "Expected M <= N");
      for (size_t i = 0; i < M; ++i) {
        m_data[i] = rhs[i];
      }
      ASSERT(rest.size() == N - M, "Vector initialised with incorrect number of values");
      std::copy(rest.begin(), rest.end(), m_data.begin() + M);
    }

    const T* data() const
    {
      return m_data.data();
    }

    T* data()
    {
      return m_data.data();
    }

    const T& operator[](size_t i) const
    {
      DBG_ASSERT(i < N, "Index out of range");
      return m_data[i];
    }

    T& operator[](size_t i)
    {
      DBG_ASSERT(i < N, "Index out of range");
      return m_data[i];
    }

    template<size_t M>
    Vector<T, M> sub() const
    {
      static_assert(M <= N, "Expected M <= N");

      Vector<T, M> v;
      for (size_t i = 0; i < M; ++i) {
        v[i] = m_data[i];
      }
      return v;
    }

    template<typename U>
    operator Vector<U, N>() const
    {
      Vector<U, N> v;
      for (size_t i = 0; i < N; ++i) {
        v[i] = static_cast<U>(m_data[i]);
      }
      return v;
    }

    Vector<T, N> operator-() const
    {
      Vector<T, N> v;
      for (size_t i = 0; i < N; ++i) {
        v[i] = -m_data[i];
      }
      return v;
    }

    Vector<T, N> operator+(const Vector<T, N>& rhs) const
    {
      Vector<T, N> v;
      for (size_t i = 0; i < N; ++i) {
        v[i] = m_data[i] + rhs[i];
      }
      return v;
    }

    Vector<T, N> operator-(const Vector<T, N>& rhs) const
    {
      Vector<T, N> v;
      for (size_t i = 0; i < N; ++i) {
        v[i] = m_data[i] - rhs[i];
      }
      return v;
    }

    Vector<T, N>& operator+=(const Vector<T, N>& rhs)
    {
      for (size_t i = 0; i < N; ++i) {
        m_data[i] += rhs.m_data[i];
      }
      return *this;
    }

    Vector<T, N>& operator-=(const Vector<T, N>& rhs)
    {
      for (size_t i = 0; i < N; ++i) {
        m_data[i] -= rhs.m_data[i];
      }
      return *this;
    }

    Vector<T, N> operator*(T s) const
    {
      Vector<T, N> v;
      for (size_t i = 0; i < N; ++i) {
        v[i] = m_data[i] * s;
      }
      return v;
    }

    Vector<T, N> operator/(T s) const
    {
      Vector<T, N> v;
      for (size_t i = 0; i < N; ++i) {
        v[i] = m_data[i] / s;
      }
      return v;
    }

    Vector<T, N> operator/(const Vector<T, N>& rhs) const
    {
      Vector<T, N> v;
      for (size_t i = 0; i < N; ++i) {
        v[i] = m_data[i] / rhs.m_data[i];
      }
      return v;
    }

    T magnitude() const
    {
      T sqSum = 0;
      for (size_t i = 0; i < N; ++i) {
        sqSum += m_data[i] * m_data[i];
      }
      return sqrt(sqSum);
    }

    Vector<T, N> normalise() const
    {
      T mag = magnitude();
      return mag != 0.0 ? (*this) / magnitude() : (*this);
    }

    Vector<T, N> inverse() const
    {
      Vector<T, N> v;
      for (size_t i = 0; i < N; ++i) {
        v.m_data[i] = -m_data[i];
      }
      return v;
    }

    bool operator==(const Vector<T, N>& rhs) const
    {
      for (size_t i = 0; i < N; ++i) {
        if (m_data[i] != rhs.data()[i]) {
          return false;
        }
      }
      return true;
    }

    bool operator!=(const Vector<T, N>& rhs) const {
      return !(*this == rhs);
    }

    T dot(const Vector<T, N>& rhs) const
    {
      T sum = 0;
      for (size_t i = 0; i < N; ++i) {
        sum += m_data[i] * rhs.m_data[i];
      }
      return sum;
    }

    Vector<T, 3> cross(const Vector<T, 3>& rhs) const
    {
      return Vector<T, N>{
        m_data[1] * rhs.m_data[2] - m_data[2] * rhs.m_data[1],
        m_data[2] * rhs.m_data[0] - m_data[0] * rhs.m_data[2],
        m_data[0] * rhs.m_data[1] - m_data[1] * rhs.m_data[0]
      };
    }

  private:
    std::array<T, N> m_data;
};

template<typename T, size_t N>
std::ostream& operator<<(std::ostream& stream, const Vector<T, N>& v)
{
  for (size_t i = 0; i < N; ++i) {
    stream << v[i] << (i + 1 < N ? ", " : "");
  }
  return stream;
}

template<typename T, size_t ROWS, size_t COLS>
class Matrix
{
  public:
    Matrix() : m_data{}
    {
    }

    Matrix(const std::initializer_list<T>& data)
    {
      if (data.size() != ROWS * COLS) {
        EXCEPTION("Expected initializer list should of length " << ROWS * COLS);
      }

      // Transpose and store
      size_t i = 0;
      for (T x : data) {
        set(i / COLS, i % COLS, x);
        ++i;
      }
    }

    Matrix(const Matrix<T, ROWS, COLS>& cp)
      : m_data(cp.m_data)
    {
    }
    
    Matrix(Matrix<T, ROWS, COLS>&& mv)
      : m_data(std::move(mv.m_data))
    {
    }

    Matrix<T, ROWS, COLS>& operator=(const Matrix<T, ROWS, COLS>& rhs)
    {
      m_data = rhs.m_data;
      return *this;
    }

    Matrix<T, ROWS, COLS>& operator=(Matrix<T, ROWS, COLS>&& rhs)
    {
      m_data = std::move(rhs.m_data);
      return *this;
    }

    const T* data() const
    {
      return m_data.data();
    }

    T* data()
    {
      return m_data.data();
    }

    const T& at(size_t row, size_t col) const
    {
      DBG_ASSERT(row < ROWS, "Index out of range");
      DBG_ASSERT(col < COLS, "Index out of range");
      return m_data[col * ROWS + row];
    }

    void set(size_t row, size_t col, T value)
    {
      DBG_ASSERT(row < ROWS, "Index out of range");
      DBG_ASSERT(col < COLS, "Index out of range");
      m_data[col * ROWS + row] = value;
    }

    template<typename U>
    operator Matrix<U, ROWS, COLS>() const
    {
      Matrix<U, ROWS, COLS> m;
      for (size_t r = 0; r < ROWS; ++r) {
        for (size_t c = 0; c < COLS; ++c) {
          m.set(r, c, static_cast<U>(at(r, c)));
        }
      }
      return m;
    }

    Matrix<T, ROWS, COLS> operator+(const Matrix<T, ROWS, COLS>& rhs) const
    {
      Matrix<T, ROWS, COLS> m;
      for (size_t i = 0; i < ROWS * COLS; ++i) {
        m.m_data[i] = m_data[i] + rhs.m_data[i];
      }
      return m;
    }

    Matrix<T, ROWS, COLS> operator*(T s) const
    {
      Matrix<T, ROWS, COLS> m;
      for (size_t i = 0; i < ROWS * COLS; ++i) {
        m.m_data[i] = m_data[i] * s;
      }
      return m;
    }

    Vector<T, ROWS> operator*(const Vector<T, COLS>& rhs) const
    {
      Vector<T, ROWS> v;
      for (size_t j = 0; j < ROWS; ++j) {
        T sum = 0;
        for (size_t i = 0; i < COLS; ++i) {
          sum += at(j, i) * rhs[i];
        }
        v[j] = sum;
      }
      return v;
    }

    template <size_t RCOLS>
    Matrix<T, ROWS, RCOLS> operator*(const Matrix<T, COLS, RCOLS>& rhs) const
    {
      Matrix<T, ROWS, RCOLS> m;
      for (size_t j = 0; j < ROWS; ++j) {
        for (size_t i = 0; i < RCOLS; ++i) {
          T sum = 0;
          for (size_t k = 0; k < COLS; ++k) {
            sum += at(j, k) * rhs.at(k, i);
          }
          m.set(j, i, sum);
        }
      }
      return m;
    }

    Matrix<T, COLS, ROWS> t() const
    {
      Matrix<T, COLS, ROWS> m;
      for (size_t i = 0; i < COLS; ++i) {
        for (size_t j = 0; j < ROWS; ++j) {
          m.set(i, j, at(j, i));
        }
      }
      return m;
    }

    bool operator==(const Matrix<T, ROWS, COLS>& rhs) const
    {
      for (size_t i = 0; i < ROWS * COLS; ++i) {
        if (m_data[i] != rhs.data()[i]) {
          return false;
        }
      }
      return true;
    }

  private:
    std::array<T, ROWS * COLS> m_data;
};

template<typename T, size_t ROWS, size_t COLS>
std::ostream& operator<<(std::ostream& stream, const Matrix<T, ROWS, COLS>& m)
{
  for (size_t r = 0; r < ROWS; ++r) {
    for (size_t c = 0; c < COLS; ++c) {
      stream << m.at(r, c) << (c + 1 < COLS ? ", " : "");
    }
    stream << "\n";
  }
  return stream;
}

using Vec2i = Vector<int, 2>;
using Vec2f = Vector<float_t, 2>;
using Vec3f = Vector<float_t, 3>;
using Vec4f = Vector<float_t, 4>;
using Mat2x2f = Matrix<float_t, 2, 2>;
using Mat3x3f = Matrix<float_t, 3, 3>;
using Mat4x4f = Matrix<float_t, 4, 4>;

template<typename T, size_t M>
Matrix<T, M, M> scaleMatrix(T scale, bool homogeneous)
{
  Matrix<T, M, M> m;
  for (size_t i = 0; i < M; ++i) {
    m.set(i, i, scale);
  }
  if (homogeneous) {
    m.set(M - 1, M - 1, 1);
  }
  return m;
};

template<typename T, size_t M>
Matrix<T, M, M> identityMatrix()
{
  return scaleMatrix<T, M>(1, false);
}

template<typename T>
Matrix<T, 3, 3> crossProductMatrix3x3(const Vector<T, 3>& k)
{
  return Matrix<T, 3, 3>{
    0, -k[2], k[1],
    k[2], 0, -k[0],
    -k[1], k[0], 0
  };
}

// Rodrigues' rotation formula
template<typename T>
Matrix<T, 3, 3> rotationMatrix3x3(const Vector<T, 3>& k, T theta)
{
  auto K = crossProductMatrix3x3(k);
  return identityMatrix<T, 3>() + K * sin(theta) + (K * K) * (1.f - cos(theta));
}

// From Euler angles
template<typename T>
Matrix<T, 3, 3> rotationMatrix3x3(const Vector<T, 3>& ori)
{
  Matrix<T, 3, 3> X{
    1, 0, 0,
    0, cosine(ori[0]), -sine(ori[0]),
    0, sine(ori[0]), cosine(ori[0])
  };
  Matrix<T, 3, 3> Y{
    cosine(ori[1]), 0, sine(ori[1]),
    0, 1, 0,
    -sine(ori[1]), 0, cosine(ori[1])
  };
  Matrix<T, 3, 3> Z{
    cosine(ori[2]), -sine(ori[2]), 0,
    sine(ori[2]), cosine(ori[2]), 0,
    0, 0, 1
  };
  return Z * (Y * X);
}

template<typename T>
Matrix<T, 4, 4> translationMatrix4x4(const Vector<T, 3>& pos)
{
  auto m = identityMatrix<T, 4>();
  m.set(0, 3, pos[0]);
  m.set(1, 3, pos[1]);
  m.set(2, 3, pos[2]);
  return m;
}

template<typename T>
Matrix<T, 4, 4> rotationMatrix4x4(const Vector<T, 3>& ori)
{
  auto rot = rotationMatrix3x3(ori);
  Matrix<T, 4, 4> m = identityMatrix<T, 4>();
  for (size_t r = 0; r < 3; ++r) {
    for (size_t c = 0; c < 3; ++c) {
      m.set(r, c, rot.at(r, c));
    }
  }
  return m;
}

template<typename T>
Matrix<T, 4, 4> transform(const Vector<T, 3>& pos, const Vector<T, 3>& ori)
{
  auto rot = rotationMatrix3x3(ori);
  Matrix<T, 4, 4> m = identityMatrix<float_t, 4>();
  for (size_t r = 0; r < 3; ++r) {
    for (size_t c = 0; c < 3; ++c) {
      m.set(r, c, rot.at(r, c));
    }
  }
  m.set(0, 3, pos[0]);
  m.set(1, 3, pos[1]);
  m.set(2, 3, pos[2]);
  return m;
}

template<typename T>
Matrix<T, 4, 4> fromVerticalToVectorTransform(const Vector<T, 3>& vec)
{
  Vector<T, 3> u = vec.normalise();
  Vector<T, 3> w = Vector<T, 3>{0, 0, 1};
  Vector<T, 3> v = w.cross(u).normalise();
  w = u.cross(v);

  return Matrix<T, 4, 4>{
    v[0], w[0], u[0], 0,
    v[1], w[1], u[1], 0,
    v[2], w[2], u[2], 0,
    0, 0, 0, 1
  };
}

template<typename T>
Matrix<T, 3, 3> getRotation3x3(const Matrix<T, 4, 4>& m)
{
  Matrix<T, 3, 3> rot;
  for (size_t r = 0; r < 3; ++r) {
    for (size_t c = 0; c < 3; ++c) {
      rot.set(r, c, m.at(r, c));
    }
  }
  return rot;
}

template<typename T>
Vector<T, 3> getTranslation(const Matrix<T, 4, 4>& m)
{
  return Vector<T, 3>{m.at(0, 3), m.at(1, 3), m.at(2, 3)};
}

template<typename T>
void setTranslation(Matrix<T, 4, 4>& m, const Vector<T, 3>& t)
{
  m.set(0, 3, t[0]);
  m.set(1, 3, t[1]);
  m.set(2, 3, t[2]);
}

struct LineSegment
{
  Vec2f A;
  Vec2f B;
};

struct Line
{
  Line(const Vec2f& A, const Vec2f& B);
  Line(float_t a, float_t b, float_t c);

  float_t a;
  float_t b;
  float_t c;
};

Mat4x4f lookAt(const Vec3f& eye, const Vec3f& centre);
Mat4x4f perspective(float_t fovY, float_t aspect, float_t near, float_t far);
bool lineIntersect(const Line& l1, const Line& l2, Vec2f& p);
Vec2f projectionOntoLine(const Line& line, const Vec2f& p);
bool lineSegmentCircleIntersect(const LineSegment& lseg, const Vec2f& centre, float_t radius);
bool pointIsInsidePoly(const Vec2f& p, const std::vector<Vec2f>& poly);
std::vector<uint16_t> triangulatePoly(const std::vector<Vec4f>& vertices);
