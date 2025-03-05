#pragma once

#include "utils.hpp"
#include <array>
#include <cassert>
#include <mutex>

template<typename T>
class TripleBuffer
{
  public:
    // Call from writer thread
    //
    T& writeComplete()
    {
      std::lock_guard lock(m_mutex);
      m_timestamps[m_writeIndex] = ++m_frameCount;
      std::swap(m_writeIndex, m_freeIndex);
      assert(inRange<size_t>(m_writeIndex, 0u, 2u));
      return m_items[m_writeIndex];
    }

    T& getWritable()
    {
      assert(inRange<size_t>(m_writeIndex, 0u, 2u));
      return m_items[m_writeIndex];
    }

    // Call from reader thread
    //
    T& readComplete()
    {
      std::lock_guard lock(m_mutex);
      if (m_timestamps[m_freeIndex] > m_timestamps[m_readIndex]) {
        std::swap(m_readIndex, m_freeIndex);
      }
      assert(inRange<size_t>(m_readIndex, 0u, 2u));
      return m_items[m_readIndex];
    }

    T& getReadable()
    {
      assert(inRange<size_t>(m_readIndex, 0u, 2u));
      return m_items[m_readIndex];
    }

  private:
    std::array<T, 3> m_items{};
    std::array<size_t, 3> m_timestamps{};

    std::mutex m_mutex;
    size_t m_writeIndex = 0;
    size_t m_readIndex = 1;
    size_t m_freeIndex = 2;
    size_t m_frameCount = 0;
};
