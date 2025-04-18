#include "time.hpp"
#include <thread>

using namespace std::chrono_literals;

FrameRateLimiter::FrameRateLimiter(unsigned frameRate)
  : m_frameDuration(1000000us / frameRate)
{
}

void FrameRateLimiter::wait()
{
  auto time = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(time - m_lastFrameTime);

  if (elapsed < m_frameDuration) {
    std::this_thread::sleep_for(m_frameDuration - elapsed);
  }

  m_lastFrameTime = std::chrono::high_resolution_clock::now();
}

Timer::Timer()
  : m_start(std::chrono::high_resolution_clock::now())
{
}

double Timer::elapsed() const
{
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double>(now - m_start).count();
}

void Timer::reset()
{
  m_start = std::chrono::high_resolution_clock::now();
}
