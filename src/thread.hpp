#pragma once

#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>

class Thread
{
  public:
    Thread()
    {
      m_thread = std::thread([this]() { loop(); });
    }

    void run(const std::function<void()>& task)
    {
      wait();
      {
        std::lock_guard lock(m_mutex);
        m_hasWork = true;
        m_task = task;
      }
      m_conditionVariable.notify_one();
    }

    void wait()
    {
      std::unique_lock lock(m_mutex);
      if (!m_hasWork) {
        return;
      }
      m_conditionVariable.wait(lock, [this]() { return !m_hasWork; });
    }

    ~Thread()
    {
      {
        std::lock_guard lock(m_mutex);
        m_running = false;
      }
      m_conditionVariable.notify_one();
      m_thread.join();
    }

  private:
    void loop()
    {
      while (true) {
        {
          std::unique_lock lock(m_mutex);
          m_conditionVariable.wait(lock, [this]() { return m_hasWork || !m_running; });

          if (!m_running) {
            return;
          }
        }

        m_task();

        {
          std::lock_guard lock(m_mutex);
          m_hasWork = false;
        }

        m_conditionVariable.notify_one();
      }
    }

    std::thread m_thread;
    bool m_running = true;
    bool m_hasWork = false;
    std::function<void()> m_task;
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
};
