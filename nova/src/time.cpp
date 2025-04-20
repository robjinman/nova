#include "time.hpp"
#include <thread>

#ifdef _WIN32
#include <windows.h>
static bool init = []() {
  timeBeginPeriod(1);
  return true;
}();
#endif

using namespace std::chrono_literals;

const double SLEEP_RATIO = 0.9;

FrameRateLimiter::FrameRateLimiter(unsigned frameRate)
  : m_frameDuration(1000000us / frameRate)
{
}

void FrameRateLimiter::wait()
{
  auto getElapsed = [this]() {
    auto time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(time - m_lastFrameTime);
  };

  auto busyWait = [&]() {
    while (getElapsed() < m_frameDuration) {}
  };

  auto elapsed = getElapsed();
  if (elapsed < m_frameDuration) {
    std::this_thread::sleep_for((m_frameDuration - elapsed) * SLEEP_RATIO);

    busyWait();
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
