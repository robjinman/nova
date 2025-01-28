#pragma once

#include "math.hpp"

class Camera
{
  public:
    void translate(const Vec3f& delta);
    void rotate(float deltaPitch, float deltaYaw);

    Mat4x4f getTransform() const;

  private:
    Mat4x4f m_transform = Mat4x4f(1);
};
