#pragma once

#include <array>
#include <type_traits>

template<>
struct std::hash<std::pair<size_t, size_t>>
{
  size_t operator()(const std::pair<size_t, size_t>& x) const noexcept
  {
    size_t seed = x.first;
    seed ^= x.second + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    return seed;
  }
};

inline size_t hashAll()
{
  return 0;
}

template<typename T, typename... Ts>
size_t hashAll(T first, Ts... rest)
{
  size_t hash1 = std::hash<T>{}(first);
  size_t hash2 = hashAll(rest...);
  return std::hash<std::pair<size_t, size_t>>{}({ hash1, hash2 });
}

template<typename T>
concept PrimitiveLike = (std::is_fundamental_v<T> && !std::is_void_v<T>) || std::is_enum_v<T>;

template<PrimitiveLike T, size_t N>
struct std::hash<std::array<T, N>>
{
  size_t operator()(const std::array<T, N>& x) const noexcept
  {
    std::string_view stringView{reinterpret_cast<const char*>(x.data()), x.size() * sizeof(T)};
    return std::hash<std::string_view>{}(stringView);
  }
};
