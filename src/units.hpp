#pragma once

#include "math.hpp"

const float_t FRAME_RATE = 60.0;
const float_t WORLD_UNITS_PER_METRE = 10;

inline float_t metresToWorldUnits(float_t x)
{
  return x * WORLD_UNITS_PER_METRE;
}

template<size_t N>
Vector<float_t, N> metresToWorldUnits(const Vector<float_t, N>& v)
{
  Vector<float_t, N> k;
  for (size_t i = 0; i < N; ++i) {
    k[i] = metresToWorldUnits(v[i]);
  }
  return k;
}
