#include "camera.hpp"

Camera::Camera()
  : m_position{0, 0, 0}
  , m_direction{0, 0, -1}
{
}

void Camera::translate(const Vec3f& delta)
{
  m_position += delta;
}

void Camera::rotate(float_t deltaPitch, float_t deltaYaw)
{
  auto pitch = rotationMatrix3x3(m_direction.cross(Vec3f{0, 1, 0}), deltaPitch);
  auto yaw = rotationMatrix3x3(Vec3f{0, 1, 0}, deltaYaw);
  m_direction = (yaw * pitch * m_direction).normalise();
}

Mat4x4f Camera::getMatrix() const
{
  return lookAt(m_position, m_position + m_direction);
}

const Vec3f& Camera::getPosition() const
{
  return m_position;
}

const Vec3f& Camera::getDirection() const
{
  return m_direction;
}
