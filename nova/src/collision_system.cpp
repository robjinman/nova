#include "collision_system.hpp"
#include "spatial_system.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "grid.hpp"
#include <array>
#include <list>
#include <set>

const size_t GRID_W = 50;
const size_t GRID_H = 50;

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
    std::unique_ptr<Grid<const CollisionItem*, GRID_W, GRID_H>> m_edgeGrid;
    std::unique_ptr<Grid<const CollisionItem*, GRID_W, GRID_H>> m_areaGrid;

    Vec3f tryMove(const Vec3f& pos, const Vec3f& delta, float_t radius, float_t stepHeight,
      int depth) const;
    std::vector<LineSegment> intersectingLineSegments(std::unordered_set<const CollisionItem*>& items,
      const Vec3f& pos3, float_t radius, float_t stepHeight) const;
};

CollisionSystemImpl::CollisionSystemImpl(const SpatialSystem& spatialSystem, Logger& logger)
  : m_logger(logger)
  , m_spatialSystem(spatialSystem)
{
}

void CollisionSystemImpl::initialise(const Vec2f& worldMin, const Vec2f& worldMax)
{
  m_edgeGrid = std::make_unique<Grid<const CollisionItem*, GRID_W, GRID_H>>(worldMin, worldMax);
  m_areaGrid = std::make_unique<Grid<const CollisionItem*, GRID_W, GRID_H>>(worldMin, worldMax);
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
  m_edgeGrid->addItemByPerimeter(m_items.back()->absPerimeter, m_items.back().get());
  m_areaGrid->addItemByArea(m_items.back()->absPerimeter, m_items.back().get());
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
  std::unordered_set<const CollisionItem*>& items, const Vec3f& pos3, float_t radius,
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

    Vec2f adjustment = toLine.normalise() * (radius - toLine.magnitude()) * 1.01f;
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
