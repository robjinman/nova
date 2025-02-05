#pragma once

#include "math.hpp"

class Player
{
  public:
    virtual const Vec3f& getPosition() const = 0;
    virtual void setPosition(const Vec3f& position) = 0;
    virtual Vec3f getDirection() const = 0;
    virtual void translate(const Vec3f& delta) = 0;
    virtual void rotate(float_t deltaYaw, float_t deltaPitch) = 0;
    virtual float_t getStepHeight() const = 0;
    virtual float_t getSpeed() const = 0;
    virtual float_t getRadius() const = 0;
    virtual float_t getJumpHeight() const = 0;

    virtual ~Player() {}
};

using PlayerPtr = std::unique_ptr<Player>;

class Camera;

PlayerPtr createPlayer(Camera& camera);
