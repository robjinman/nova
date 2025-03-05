#pragma once

#include "math.hpp"

class Camera
{
  public:
    Camera();

    void setPosition(const Vec3f& position);
    void translate(const Vec3f& delta);
    void rotate(float_t deltaPitch, float_t deltaYaw);
    const Vec3f& getDirection() const;
    const Vec3f& getPosition() const;
    Mat4x4f  getMatrix() const;

  private:
    Vec3f m_position;
    Vec3f m_direction;
};
