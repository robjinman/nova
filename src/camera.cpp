#include "camera.hpp"

void Camera::translate(const Vec3f& delta)
{
  m_transform = glm::translate(m_transform, delta);
}

void Camera::rotate(float deltaPitch, float deltaYaw)
{
  auto rotY = glm::rotate(Mat4x4f(1), deltaYaw, Vec3f(0.f, 1.f, 0.f));
  m_transform = glm::rotate(rotY * m_transform, deltaPitch, Vec3f(1.f, 0.f, 0.f));
}

Mat4x4f Camera::getTransform() const
{
  return m_transform;
}
