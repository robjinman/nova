#pragma once

#include "math.hpp"
#include <unordered_set>
#include <array>
#include <cassert>

struct Vec2iHash
{
  inline size_t operator()(const Vec2i& value) const
  {
    int64_t a = static_cast<int64_t>(value[0]) << 32;
    int64_t b = static_cast<int64_t>(value[1]);
    return std::hash<int64_t>{}(a | b);
  }
};

using GridCellList = std::unordered_set<Vec2i, Vec2iHash>;

template<typename T, size_t GRID_W, size_t GRID_H>
class Grid
{
  static_assert(GRID_W > 0, "Grid width must be greater than 0");
  static_assert(GRID_H > 0, "Grid height must be greater than 0");

  public:
    Grid(const Vec2f& worldMin, const Vec2f& worldMax);

    void addItemByPerimeter(const std::vector<Vec2f>& poly, const T& item);
    void addItemByArea(const std::vector<Vec2f>& poly, const T& item);
    void addItemByRadius(const Vec2f& pos, float_t radius, const T& item);

    std::unordered_set<T> getItems(const Vec2f& pos, float_t radius) const;
    std::unordered_set<T> getItems(const Vec2f& pos) const;
    std::unordered_set<T> getItems(const std::vector<Vec2f>& poly) const;

    // Exposed for testing
    //
    GridCellList test_gridCellsBetweenPoints(const Vec2f& A, const Vec2f& B) const;

  private:
    Vec2f m_worldMin;
    Vec2f m_worldMax;
    float_t m_worldW;
    float_t m_worldH;
    float_t m_cellW;
    float_t m_cellH;
    std::array<std::array<std::unordered_set<T>, GRID_H>, GRID_W> m_items;

    bool withinBounds(const Vec2f& p) const;
    void boundsCheck(const Vec2f& p) const;
    Vec2i worldToGridCoords(const Vec2f& p) const;
    GridCellList gridCellsBetweenPoints(const Vec2f& A, const Vec2f& B) const;
    bool cellInRange(const Vec2i& cell) const;
};

template<typename T, size_t GRID_W, size_t GRID_H>
Grid<T, GRID_W, GRID_H>::Grid(const Vec2f& worldMin, const Vec2f& worldMax)
  : m_worldMin(worldMin)
  , m_worldMax(worldMax)
  , m_worldW(m_worldMax[0] - m_worldMin[0])
  , m_worldH(m_worldMax[1] - m_worldMin[1])
  , m_cellW(m_worldW / GRID_W)
  , m_cellH(m_worldH / GRID_H)
{
}

template<typename T, size_t GRID_W, size_t GRID_H>
void Grid<T, GRID_W, GRID_H>::addItemByPerimeter(const std::vector<Vec2f>& poly, const T& item)
{
  if (poly.size() == 0) {
    return;
  }

  const size_t n = poly.size();
  for (size_t i = 0; i < n; ++i) {
    auto& p1 = poly[i];
    auto& p2 = poly[(i + 1) % n];

    auto cells = gridCellsBetweenPoints(p1, p2);
    for (auto& cell : cells) {
      if (cellInRange(cell)) {
        m_items[cell[0]][cell[1]].insert(item);
      }
    }
  }
}

template<typename T, size_t GRID_W, size_t GRID_H>
void Grid<T, GRID_W, GRID_H>::addItemByArea(const std::vector<Vec2f>& poly, const T& item)
{
  if (poly.size() == 0) {
    return;
  }

  addItemByPerimeter(poly, item);

  // TODO: Too slow?
  for (size_t i = 0; i < GRID_W; ++i) {
    for (size_t j = 0; j < GRID_H; ++j) {
      Vec2f cellCentre{
        m_worldMin[0] + (static_cast<float_t>(i) + 0.5f) * m_cellW,
        m_worldMin[1] + (static_cast<float_t>(j) + 0.5f) * m_cellH
      };

      if (pointIsInsidePoly(cellCentre, poly)) {
        m_items[i][j].insert(item);
      }
    }
  }
}

template<typename T, size_t GRID_W, size_t GRID_H>
void Grid<T, GRID_W, GRID_H>::addItemByRadius(const Vec2f& pos, float_t radius, const T& item)
{
  boundsCheck(pos);

  Vec2i p0 = worldToGridCoords(Vec2f{ pos[0] - radius, pos[1] - radius });
  Vec2i p1 = worldToGridCoords(Vec2f{ pos[0] + radius, pos[1] + radius });

  for (int i = std::max(0, p0[0]); i <= std::min(p1[0], static_cast<int>(GRID_W - 1)); ++i) {
    for (int j = std::max(0, p0[1]); j <= std::min(p1[1], static_cast<int>(GRID_H - 1)); ++j) {
      m_items[i][j].insert(item);
    }
  }
}

template<typename T, size_t GRID_W, size_t GRID_H>
std::unordered_set<T> Grid<T, GRID_W, GRID_H>::getItems(const Vec2f& pos, float_t radius) const
{
  std::unordered_set<T> items;

  Vec2i p0 = worldToGridCoords(Vec2f{ pos[0] - radius, pos[1] - radius });
  Vec2i p1 = worldToGridCoords(Vec2f{ pos[0] + radius, pos[1] + radius });

  for (int i = std::max(0, p0[0]); i <= std::min(p1[0], static_cast<int>(GRID_W - 1)); ++i) {
    for (int j = std::max(0, p0[1]); j <= std::min(p1[1], static_cast<int>(GRID_H - 1)); ++j) {
      auto& list = m_items[i][j];
      items.insert(list.begin(), list.end());
    }
  }

  return items;
}

template<typename T, size_t GRID_W, size_t GRID_H>
std::unordered_set<T> Grid<T, GRID_W, GRID_H>::getItems(const Vec2f& pos) const
{
  boundsCheck(pos);

  Vec2i p = worldToGridCoords(pos);

  return m_items[p[0]][p[1]];
}

template<typename T, size_t GRID_W, size_t GRID_H>
std::unordered_set<T> Grid<T, GRID_W, GRID_H>::getItems(const std::vector<Vec2f>& poly) const
{
  std::unordered_set<T> items;

  if (poly.size() == 0) {
    return items;
  }

  Vec2i minGridCoord{ GRID_W - 1, GRID_H - 1 };
  Vec2i maxGridCoord{ 0, 0 };

  const size_t n = poly.size();
  for (size_t i = 0; i < n; ++i) {
    auto& p1 = poly[i];
    auto& p2 = poly[(i + 1) % n];

    auto cells = gridCellsBetweenPoints(p1, p2);
    for (auto& cell : cells) {
      if (cellInRange(cell)) {
        auto& list = m_items[cell[0]][cell[1]];
        items.insert(list.begin(), list.end());
      }

      if (cell[0] < minGridCoord[0]) {
        minGridCoord[0] = std::max(cell[0], 0);
      }
      if (cell[0] > maxGridCoord[0]) {
        maxGridCoord[0] = std::min(cell[0], static_cast<int>(GRID_W - 1));
      }
      if (cell[1] < minGridCoord[1]) {
        minGridCoord[1] = std::max(cell[1], 0);
      }
      if (cell[1] > maxGridCoord[1]) {
        maxGridCoord[1] = std::min(cell[1], static_cast<int>(GRID_H - 1));
      }
    }
  }

  for (int i = minGridCoord[0]; i <= maxGridCoord[0]; ++i) {
    for (int j = minGridCoord[1]; j <= maxGridCoord[1]; ++j) {
      Vec2f cellCentre{
        m_worldMin[0] + (static_cast<float_t>(i) + 0.5f) * m_cellW,
        m_worldMin[1] + (static_cast<float_t>(j) + 0.5f) * m_cellH
      };

      if (pointIsInsidePoly(cellCentre, poly)) {
        auto& list = m_items[i][j];
        items.insert(list.begin(), list.end());
      }
    }
  }

  return items;
}

template<typename T, size_t GRID_W, size_t GRID_H>
bool Grid<T, GRID_W, GRID_H>::cellInRange(const Vec2i& cell) const
{
  return cell[0] >= 0 && cell[0] < static_cast<int>(GRID_W) &&
    cell[1] >= 0 && cell[1] < static_cast<int>(GRID_H);
}

template<typename T, size_t GRID_W, size_t GRID_H>
bool Grid<T, GRID_W, GRID_H>::withinBounds(const Vec2f& p) const
{
  if (p[0] < m_worldMin[0] || p[0] > m_worldMax[0]) { return false; }
  if (p[1] < m_worldMin[1] || p[1] > m_worldMax[1]) { return false; }
  return true;
}

template<typename T, size_t GRID_W, size_t GRID_H>
void Grid<T, GRID_W, GRID_H>::boundsCheck(const Vec2f& p) const
{
  ASSERT(withinBounds(p), "Point (" << p << ") out of bounds");
}

template<typename T, size_t GRID_W, size_t GRID_H>
Vec2i Grid<T, GRID_W, GRID_H>::worldToGridCoords(const Vec2f& p) const
{
  return {
    static_cast<int>(floor((p[0] - m_worldMin[0]) / m_cellW)),
    static_cast<int>(floor((p[1] - m_worldMin[1]) / m_cellH))
  };
}

template<typename T, size_t GRID_W, size_t GRID_H>
GridCellList Grid<T, GRID_W, GRID_H>::gridCellsBetweenPoints(const Vec2f& A, const Vec2f& B) const
{
  GridCellList cells;

  Vec2i startCell = worldToGridCoords(A);
  Vec2i endCell = worldToGridCoords(B);

  cells.insert(startCell);

  if (startCell == endCell) {
    return cells;
  }

  int stepX = B[0] > A[0] ? 1 : -1;
  int stepY = B[1] > A[1] ? 1 : -1;

  Vec2f delta = B - A;

  float_t nextVertical = m_worldMin[0] + m_cellW * (startCell[0] + (stepX > 0 ? 1 : 0));
  float_t nextHorizontal = m_worldMin[1] + m_cellH * (startCell[1] + (stepY > 0 ? 1 : 0));

  float_t tx = fabs(delta[0]) > 0.f ?
    (nextVertical - A[0]) / delta[0] :
    std::numeric_limits<float_t>::max();

  float_t ty = fabs(delta[1]) > 0 ?
    (nextHorizontal - A[1]) / delta[1] :
    std::numeric_limits<float_t>::max();

  assert(tx >= 0);
  assert(ty >= 0);  // TODO: Correct?

  float_t dtX = m_cellW / fabs(delta[0]);
  float_t dtY = m_cellH / fabs(delta[1]);

  Vec2i cell = startCell;

  while (cell != endCell) {
    if (tx < ty) {
      cell[0] += stepX;
      tx += dtX;
    }
    else {
      cell[1] += stepY;
      ty += dtY;
    }

    cells.insert(cell);
  }

  return cells;
}

template<typename T, size_t GRID_W, size_t GRID_H>
GridCellList Grid<T, GRID_W, GRID_H>::test_gridCellsBetweenPoints(const Vec2f& A,
  const Vec2f& B) const
{
  return gridCellsBetweenPoints(A, B);
}
