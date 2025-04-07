#include <grid.hpp>
#include <gtest/gtest.h>

class GridTest : public testing::Test
{
  public:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

TEST_F(GridTest, gridCellsBetweenPoints_single_cell)
{
  Grid<char, 10, 10> grid(Vec2f{ 0, 0 }, Vec2f{ 10.f, 10.f });

  auto cells = grid.test_gridCellsBetweenPoints({ 0.2f, 0.3f }, { 0.4f, 0.5f });

  GridCellList expected{
    { 0, 0 }
  };

  EXPECT_EQ(expected, cells);
}

TEST_F(GridTest, gridCellsBetweenPoints_two_cells)
{
  Grid<char, 10, 10> grid(Vec2f{ 0, 0 }, Vec2f{ 10.f, 10.f });

  auto cells = grid.test_gridCellsBetweenPoints({ 0.2f, 0.3f }, { 1.2f, 0.9f });

  GridCellList expected{
    { 0, 0 },
    { 1, 0 }
  };

  EXPECT_EQ(expected, cells);
}

TEST_F(GridTest, gridCellsBetweenPoints_nonzero_origin)
{
  Grid<char, 10, 10> grid(Vec2f{ -5.f, -5.f }, Vec2f{ 5.f, 5.f });

  auto cells = grid.test_gridCellsBetweenPoints({ 0.2f, 0.3f }, { 1.2f, 0.9f });

  GridCellList expected{
    { 5, 5 },
    { 6, 5 }
  };

  EXPECT_EQ(expected, cells);
}

TEST_F(GridTest, gridCellsBetweenPoints_vertical_line)
{
  Grid<char, 10, 10> grid(Vec2f{ 0.f, 0.f }, Vec2f{ 10.f, 10.f });

  auto cells = grid.test_gridCellsBetweenPoints({ 0.5f, 0.5f }, { 0.5f, 7.5f });

  GridCellList expected{
    { 0, 0 },
    { 0, 1 },
    { 0, 2 },
    { 0, 3 },
    { 0, 4 },
    { 0, 5 },
    { 0, 6 },
    { 0, 7 }
  };

  EXPECT_EQ(expected, cells);
}

TEST_F(GridTest, gridCellsBetweenPoints_out_of_bounds)
{
  Grid<char, 10, 10> grid(Vec2f{ 0.f, 0.f }, Vec2f{ 10.f, 10.f });

  auto cells = grid.test_gridCellsBetweenPoints({ -0.1f, 8.1f }, { 1.8f, 10.3f });

  GridCellList expected{
    { 0, 8 },
    { 0, 9 },
    { 1, 9 }
  };

  EXPECT_EQ(expected, cells);
}
