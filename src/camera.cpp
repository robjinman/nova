#include "camera.hpp"

Camera::Camera()
  : m_position(0, 0, 0)
  , m_direction(0, 0, -1)
{
}

void Camera::translate(const Vec3f& delta)
{
  m_position += delta;
}

void Camera::rotate(float deltaPitch, float deltaYaw)
{
  static const Vec3f vertical = Vec3f{0, 1, 0};

  Vec4f v{m_direction, 1};
  auto pitch = glm::rotate(Mat4x4f(1), deltaPitch, glm::cross(m_direction, vertical));
  auto yaw = glm::rotate(Mat4x4f(1), deltaYaw, vertical);
  m_direction = glm::normalize(yaw * pitch * v);
}

Mat4x4f Camera::getMatrix() const
{
  return glm::lookAt(m_position, m_position + m_direction, Vec3f{0, 1, 0});
}

const Vec3f& Camera::getPosition() const
{
  return m_position;
}

const Vec3f& Camera::getDirection() const
{
  return m_direction;
}
