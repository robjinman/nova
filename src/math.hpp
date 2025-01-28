#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

using float_t = float;

using Mat4x4f = glm::mat<4, 4, float_t, glm::highp>;
using Vec3f = glm::vec<3, float_t, glm::highp>;
using Vec2f = glm::vec<2, float_t, glm::highp>;
