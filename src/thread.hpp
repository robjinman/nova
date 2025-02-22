#pragma once

#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <future>
#include <queue>
#include <cassert>
#include <memory>

class Thread
{
  public:
    Thread()
    {
      m_thread = std::thread([this]() { loop(); });
    }

    template<typename T>
    std::future<T> run(const std::function<T()>& task)
    {
      auto packagedTask = std::make_shared<std::packaged_task<T()>>(task);
      std::future<T> future = packagedTask->get_future();

      {
        std::lock_guard lock(m_mutex);

        m_tasks.push([packagedTask]() { (*packagedTask)(); });
        m_hasWork = true;
      }

      m_conditionVariable.notify_one();

      return future;
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
        std::function<void()> task;

        {
          std::unique_lock lock(m_mutex);
          m_conditionVariable.wait(lock, [this]() { return m_hasWork || !m_running; });

          if (!m_running) {
            return;
          }

          assert(!m_tasks.empty());

          task = std::move(m_tasks.front());
          m_tasks.pop();

          m_hasWork = !m_tasks.empty();
        }

        task();
      }
    }

    std::thread m_thread;
    bool m_running = true;
    bool m_hasWork = false;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
};
