#include "collision_system.hpp"
#include "spatial_system.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include <array>
#include <list>
#include <set>

CCollision::CCollision(EntityId entityId)
  : Component(entityId)
{
}

CCollision::CCollision(EntityId entityId, const CCollision& cpy)
  : Component(entityId)
  , perimeter(cpy.perimeter)
  , height(cpy.height)
{
}

namespace
{

const int GRID_W = 100;
const int GRID_H = 100;

struct CollisionItem
{
  CollisionItem(CCollisionPtr volume);

  CCollisionPtr volume;

  // In world space
  std::vector<Vec2f> absPerimeter;
  float_t absHeight;
};

using CollisionItemPtr = std::unique_ptr<CollisionItem>;

CollisionItem::CollisionItem(CCollisionPtr volume)
  : volume(std::move(volume))
{
}

class Grid
{
  public:
    Grid(const Vec2f& worldMin, const Vec2f& worldMax, Logger& logger);

    void addItemByPerimeter(const CollisionItem* item);
    void addItemByArea(const CollisionItem* item);
    std::set<const CollisionItem*> getItems(const Vec2f& pos, float_t radius) const;
    std::set<const CollisionItem*> getItems(const Vec2f& pos) const;

  private:
    Logger& m_logger;
    Vec2f m_worldMin;
    Vec2f m_worldMax;
    float_t m_worldW;
    float_t m_worldH;
    float_t m_cellW;
    float_t m_cellH;
    std::array<std::array<std::set<const CollisionItem*>, GRID_H>, GRID_W> m_items;

    void boundsCheck(const Vec2f& p) const;
    std::set<std::pair<int, int>> gridCellsBetweenPoints(const Vec2f& A, const Vec2f& B) const;
    Vec2i worldToGridCoords(const Vec2f& world) const;
};

Grid::Grid(const Vec2f& worldMin, const Vec2f& worldMax, Logger& logger)
  : m_logger(logger)
  , m_worldMin(worldMin)
  , m_worldMax(worldMax)
  , m_worldW(m_worldMax[0] - m_worldMin[0])
  , m_worldH(m_worldMax[1] - m_worldMin[1])
  , m_cellW(m_worldW / GRID_W)
  , m_cellH(m_worldH / GRID_H)
{
}

void Grid::boundsCheck(const Vec2f& p) const
{
  ASSERT(p[0] >= m_worldMin[0] && p[0] <= m_worldMax[0], "Point (" << p << ") is out of bounds");
  ASSERT(p[1] >= m_worldMin[1] && p[1] <= m_worldMax[1], "Point (" << p << ") is out of bounds");
}

Vec2i Grid::worldToGridCoords(const Vec2f& p) const
{
  return {
    static_cast<int>(((p[0] - m_worldMin[0]) / m_worldW) * GRID_W),
    static_cast<int>(((p[1] - m_worldMin[1]) / m_worldH) * GRID_H)
  };
}

std::set<std::pair<int, int>> Grid::gridCellsBetweenPoints(const Vec2f& A, const Vec2f& B) const
{
  boundsCheck(A);
  boundsCheck(B);

  std::set<std::pair<int, int>> cells;

  Vec2f dir = (B - A).normalise();
  float_t stepSize = std::min(m_cellW, m_cellH);
  int steps = 1 + static_cast<int>((B - A).magnitude() / stepSize);

  Vec2f P = A;
  for (int i = 0; i < steps; ++i) {
    Vec2i gridP{
      static_cast<int>((P[0] - m_worldMin[0]) / m_cellW),
      static_cast<int>((P[1] - m_worldMin[1]) / m_cellH)
    };
    cells.insert({ clip(gridP[0], 0, GRID_W - 1), clip(gridP[1], 0, GRID_H - 1) });
    P += dir * stepSize;
  }

  return cells;
}

void Grid::addItemByPerimeter(const CollisionItem* item)
{
  if (item->absPerimeter.size() == 0) {
    return;
  }

  const size_t n = item->absPerimeter.size();
  for (size_t i = 0; i < n; ++i) {
    auto& p1 = item->absPerimeter[i];
    auto& p2 = item->absPerimeter[(i + 1) % n];

    auto cells = gridCellsBetweenPoints(p1, p2);
    for (auto& cell : cells) {
      m_items[cell.first][cell.second].insert(item);
    }
  }
}

void Grid::addItemByArea(const CollisionItem* item)
{
  if (item->absPerimeter.size() == 0) {
    return;
  }

  addItemByPerimeter(item);

  for (size_t i = 0; i < GRID_W; ++i) {
    for (size_t j = 0; j < GRID_H; ++j) {
      Vec2f cellCentre{
        m_worldMin[0] + (static_cast<float_t>(i) + 0.5f) * m_cellW,
        m_worldMin[1] + (static_cast<float_t>(j) + 0.5f) * m_cellH
      };

      if (pointIsInsidePoly(cellCentre, item->absPerimeter)) {
        m_items[i][j].insert(item);
      }
    }
  }
}

std::set<const CollisionItem*> Grid::getItems(const Vec2f& pos, float_t radius) const
{
  boundsCheck(pos);

  std::set<const CollisionItem*> items;

  Vec2i p0 = worldToGridCoords(Vec2f{ pos[0] - radius, pos[1] - radius });
  Vec2i p1 = worldToGridCoords(Vec2f{ pos[0] + radius, pos[1] + radius });

  for (int i = std::max(0, p0[0]); i <= std::min(p1[0], GRID_W - 1); ++i) {
    for (int j = std::max(0, p0[1]); j <= std::min(p1[1], GRID_H - 1); ++j) {
      auto& list = m_items[i][j];
      for (auto item : list) {
        items.insert(item);
      }
    }
  }

  return items;
}

std::set<const CollisionItem*> Grid::getItems(const Vec2f& pos) const
{
  boundsCheck(pos);

  Vec2i p = worldToGridCoords(pos);

  return m_items[p[0]][p[1]];
}

class CollisionSystemImpl : public CollisionSystem
{
  public:
    CollisionSystemImpl(const SpatialSystem& spatialSystem, Logger& logger);

    void initialise(const Vec2f& worldMin, const Vec2f& worldMax) override;
    Vec3f tryMove(const Vec3f& pos, const Vec3f& delta, float_t radius,
      float_t stepHeight) const override;
    float_t altitude(const Vec3f& pos) const override;

    void update() override;
    void addComponent(ComponentPtr component) override;
    void removeComponent(EntityId entityId) override;
    bool hasComponent(EntityId entityId) const override;
    CCollision& getComponent(EntityId entityId) override;
    const CCollision& getComponent(EntityId entityId) const override;

  private:
    Logger& m_logger;
    const SpatialSystem& m_spatialSystem;
    std::list<CollisionItemPtr> m_items;
    std::unique_ptr<Grid> m_edgeGrid;
    std::unique_ptr<Grid> m_areaGrid;

    Vec3f tryMove(const Vec3f& pos, const Vec3f& delta, float_t radius, float_t stepHeight,
      int depth) const;
    std::vector<LineSegment> intersectingLineSegments(std::set<const CollisionItem*>& items,
      const Vec3f& pos3, float_t radius, float_t stepHeight) const;
};

CollisionSystemImpl::CollisionSystemImpl(const SpatialSystem& spatialSystem, Logger& logger)
  : m_logger(logger)
  , m_spatialSystem(spatialSystem)
{
}

void CollisionSystemImpl::initialise(const Vec2f& worldMin, const Vec2f& worldMax)
{
  m_edgeGrid = std::make_unique<Grid>(worldMin, worldMax, m_logger);
  m_areaGrid = std::make_unique<Grid>(worldMin, worldMax, m_logger);
}

void CollisionSystemImpl::update()
{
}

void CollisionSystemImpl::removeComponent(EntityId)
{
  // TODO
  EXCEPTION("Not implemented");
}

bool CollisionSystemImpl::hasComponent(EntityId) const
{
  // TODO
  EXCEPTION("Not implemented");
}

CCollision& CollisionSystemImpl::getComponent(EntityId)
{
  // TODO
  EXCEPTION("Not implemented");
}

const CCollision& CollisionSystemImpl::getComponent(EntityId) const
{
  // TODO
  EXCEPTION("Not implemented");
}

void CollisionSystemImpl::addComponent(ComponentPtr component)
{
  ASSERT(m_edgeGrid, "Collision system not initialised");

  auto spatialComp = m_spatialSystem.getComponent(component->id());
  auto collisionComp = CCollisionPtr(dynamic_cast<CCollision*>(component.release()));
  auto item = std::make_unique<CollisionItem>(std::move(collisionComp));

  for (auto& p : item->volume->perimeter) {
    Vec4f transformedP = spatialComp.absTransform() * Vec4f{ p[0], item->volume->height, p[1], 1 };
    item->absPerimeter.push_back({ transformedP[0], transformedP[2] });
    item->absHeight = transformedP[1];
  }

  m_items.push_back(std::move(item));
  m_edgeGrid->addItemByPerimeter(m_items.back().get());
  m_areaGrid->addItemByArea(m_items.back().get());
}

Vec3f CollisionSystemImpl::tryMove(const Vec3f& pos, const Vec3f& dir, float_t radius,
  float_t stepHeight) const
{
  ASSERT(m_edgeGrid, "Collision system not initialised");

  return tryMove(pos, dir, radius, stepHeight, 0);
}

float_t CollisionSystemImpl::altitude(const Vec3f& pos3) const
{
  Vec2f pos{ pos3[0], pos3[2] };

  auto items = m_areaGrid->getItems(pos);
  float_t highestFloor = std::numeric_limits<float_t>::lowest();

  for (auto& item : items) {
    if (pointIsInsidePoly(pos, item->absPerimeter)) {
      float_t h = item->absHeight;
      if (h > highestFloor) {
        highestFloor = h;
      }
    }
  }

  if (highestFloor == std::numeric_limits<float_t>::lowest()) {
    EXCEPTION("Player is not inside any collision volume");
  }

  return pos3[1] - highestFloor;
}

std::vector<LineSegment> CollisionSystemImpl::intersectingLineSegments(
  std::set<const CollisionItem*>& items, const Vec3f& pos3, float_t radius,
  float_t stepHeight) const
{
  Vec2f pos{ pos3[0], pos3[2] };
  std::vector<LineSegment> lineSegments;

  auto permitsEntry = [stepHeight](const CollisionItem& item, const Vec3f& pos) {
    return item.absHeight - pos[1] <= stepHeight;
  };

  for (auto item : items) {
    if (permitsEntry(*item, pos3)) {
      continue;
    }

    const size_t n = item->absPerimeter.size();
    for (size_t i = 0; i < n; ++i) {
      auto& p1 = item->absPerimeter[i];
      auto& p2 = item->absPerimeter[(i + 1) % n];

      LineSegment lseg{ p1, p2 };
      if (lineSegmentCircleIntersect(lseg, pos, radius)) {
        lineSegments.push_back(lseg);
      }
    }
  }

  return lineSegments;
}

Vec3f CollisionSystemImpl::tryMove(const Vec3f& pos3, const Vec3f& delta, float_t radius,
  float_t stepHeight, int depth) const
{
  if (depth > 10) {
    m_logger.warn("Max depth reached in CollisionSystemImpl::tryMove()");
    return Vec3f{};
  }

  Vec2f pos{ pos3[0], pos3[2] };

  Vec3f nextPos3 = pos3 + delta;
  Vec2f nextPos{ nextPos3[0], nextPos3[2] };

  auto items = m_edgeGrid->getItems(nextPos, radius);
  auto lineSegments = intersectingLineSegments(items, nextPos3, radius, stepHeight);

  float_t smallestAdjustment = std::numeric_limits<float_t>::max();
  Vec3f finalDelta = delta;

  for (auto& lseg : lineSegments) {
    Line line(lseg.A, lseg.B);
    Vec2f X = projectionOntoLine(line, nextPos);
    Vec2f toLine = nextPos - X;

    Vec2f adjustment = toLine.normalise() * (radius - toLine.magnitude()) * 1.0001f;
    Vec3f adjustment3{ adjustment[0], 0, adjustment[1] };

    auto newDelta = tryMove(pos3, delta + adjustment3, radius, stepHeight, depth + 1);

    float_t adjustmentSize = (newDelta - delta).magnitude();

    if (adjustmentSize < smallestAdjustment) {
      finalDelta = newDelta;
      smallestAdjustment = adjustmentSize;
    }
  }

  return finalDelta;
}

} // namespace

CollisionSystemPtr createCollisionSystem(const SpatialSystem& spatialSystem, Logger& logger)
{
  return std::make_unique<CollisionSystemImpl>(spatialSystem, logger);
}
