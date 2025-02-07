#include "player.hpp"
#include "camera.hpp"
#include "units.hpp"

namespace
{

class PlayerImpl : public Player
{
  public:
    PlayerImpl(Camera& camera);

    const Vec3f& getPosition() const override;
    void setPosition(const Vec3f& position) override;
    Vec3f getDirection() const override;
    virtual void translate(const Vec3f& delta) override;
    virtual void rotate(float_t deltaYaw, float_t deltaPitch) override;
    float_t getStepHeight() const override;
    float_t getSpeed() const override;
    float_t getRadius() const override;
    float_t getJumpHeight() const override;

  private:
    Camera& m_camera;
    Vec3f m_position;
    float_t m_speed = metresToWorldUnits(9.0);
    float_t m_radius = metresToWorldUnits(0.2);
    float_t m_tallness = metresToWorldUnits(1.7);
    float_t m_stepHeight = metresToWorldUnits(0.3);
    float_t m_bounceHeight = metresToWorldUnits(0.035);
    float_t m_bounceRate = 3.5;
    float_t m_jumpHeight = metresToWorldUnits(0.6);

    float_t m_originalTallness;
    float_t m_distance = 0;
};

PlayerImpl::PlayerImpl(Camera& camera)
  : m_camera(camera)
{
  m_originalTallness = m_tallness;
}

const Vec3f& PlayerImpl::getPosition() const
{
  return m_position;
}

Vec3f PlayerImpl::getDirection() const
{
  return m_camera.getDirection();
}

void PlayerImpl::translate(const Vec3f& delta)
{
  const float_t dx = m_bounceRate * 2 * PI / FRAME_RATE;

  if (delta[0] != 0 || delta[2] != 0) {
    m_tallness = m_originalTallness + m_bounceHeight * sin(m_distance);
    m_distance += dx;
  }

  m_position += delta;
  m_camera.setPosition(m_position + Vec3f{ 0, m_tallness, 0 });
}

void PlayerImpl::rotate(float_t deltaPitch, float_t deltaYaw)
{
  m_camera.rotate(deltaPitch, deltaYaw);
}

void PlayerImpl::setPosition(const Vec3f& position)
{
  m_position = position;
  m_camera.setPosition(m_position + Vec3f{ 0, m_tallness, 0 });
}

float_t PlayerImpl::getStepHeight() const
{
  return m_stepHeight;
}

float_t PlayerImpl::getSpeed() const
{
  return m_speed;
}

float_t PlayerImpl::getRadius() const
{
  return m_radius;
}

float_t PlayerImpl::getJumpHeight() const
{
  return m_jumpHeight;
}

} // namespace

PlayerPtr createPlayer(Camera& camera)
{
  return std::make_unique<PlayerImpl>(camera);
}
