#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ostream>

using float_t = float;

using Mat4x4f = glm::mat<4, 4, float_t, glm::highp>;
using Vec4f = glm::vec<4, float_t, glm::highp>;
using Vec3f = glm::vec<3, float_t, glm::highp>;
using Vec2f = glm::vec<2, float_t, glm::highp>;
using Vec2i = glm::vec<2, int>;

template<glm::length_t N, typename T, glm::qualifier Q>
std::ostream& operator<<(std::ostream& stream, const glm::vec<N, T, Q>& v)
{
  for (glm::length_t i = 0; i < N; ++i) {
    stream << v[i] << (i + 1 < N ? ", " : "");
  }
  return stream;
}
