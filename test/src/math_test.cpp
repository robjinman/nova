#include <math.hpp>
#include <utils.hpp>
#include <gtest/gtest.h>
#include <vector>

class MathTest : public testing::Test
{
  public:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

TEST_F(MathTest, matrix_data_is_column_major)
{
  Matrix<int, 2, 3> A{
    1, 2, 3,
    4, 5, 6
  };

  std::vector<int> data(A.data(), A.data() + 6);
  std::vector<int> X({1, 4, 2, 5, 3, 6});

  ASSERT_EQ(X, data);
}

TEST_F(MathTest, square_matrix_multiply)
{
  Matrix<int, 2, 2> A{
    1, 2,
    3, 4
  };

  Matrix<int, 2, 2> B{
    4, 3,
    2, 1
  };

  Matrix<int, 2, 2> C = A * B;

  Matrix<int, 2, 2> X{
    8, 5,
    20, 13
  };

  ASSERT_EQ(X, C);
}

TEST_F(MathTest, matrix_multiply_unequal_dims)
{
  Matrix<int, 2, 3> A{
    1, 2, 3,
    5, 1, 2
  };

  Matrix<int, 3, 2> B{
    4, 3,
    2, 1,
    7, 5
  };

  Matrix<int, 2, 2> C = A * B;

  Matrix<int, 2, 2> X{
    29, 20,
    36, 26
  };

  ASSERT_EQ(X, C);
}

TEST_F(MathTest, identity_matrix)
{
  auto I = identityMatrix<int, 4>();
  Matrix<int, 4, 4> m{
    5, 4, 3, 2,
    1, 2, 3, 4,
    0, 3, 3, 5,
    1, 4, 5, 8
  };

  ASSERT_EQ(m, I * m);
}

TEST_F(MathTest, cross_product_matrix)
{
  Vector<int, 3> a{5, 7, 6};
  Vector<int, 3> b{4, 3, 2};

  auto m = crossProductMatrix3x3(a);

  ASSERT_EQ(a.cross(b), m * b);
}

TEST_F(MathTest, lineSegmentCircleIntersect_true_for_point_at_end_point)
{
  LineSegment lseg{{20, 30}, {-10, 60}};
  Vec2f p = lseg.A;
  ASSERT_TRUE(lineSegmentCircleIntersect(lseg, p, 1));
}

TEST_F(MathTest, lineSegmentCircleIntersect_example_0)
{
  LineSegment lseg{{20, 30}, {-10, 60}};
  float_t radius = 6;
  Vec2f p{25, 28};
  ASSERT_TRUE(lineSegmentCircleIntersect(lseg, p, radius));
}

TEST_F(MathTest, lineSegmentCircleIntersect_example_1)
{
  LineSegment lseg{{20, 30}, {-10, 60}};
  float_t radius = 6;
  Vec2f p{25, 28};
  ASSERT_TRUE(lineSegmentCircleIntersect(lseg, p, radius));
}

TEST_F(MathTest, lineSegmentCircleIntersect_example_2)
{
  LineSegment lseg{{20, 30}, {-10, 60}};
  float_t radius = 5;
  Vec2f p{25, 28};
  ASSERT_FALSE(lineSegmentCircleIntersect(lseg, p, radius));
}

TEST_F(MathTest, clip_int_clips_to_min)
{
  ASSERT_EQ(-10, clip(-15, -10, 10));
}

TEST_F(MathTest, clip_int_clips_to_max)
{
  ASSERT_EQ(10, clip(17, -10, 10));
}

TEST_F(MathTest, clip_int_value_in_range_is_unchanged)
{
  ASSERT_EQ(5, clip(5, -10, 10));
}

TEST_F(MathTest, triangulate_square)
{
  std::vector<Vec4f> vertices{
    { 0, 0, 0, 1 },
    { 1, 0, 0, 1 },
    { 1, 0, 1, 1 },
    { 0, 0, 1, 1 }
  };

  auto indices = triangulatePoly(vertices);
  std::vector<uint16_t> expected{ 0, 1, 2, 0, 2, 3 };

  ASSERT_EQ(expected, indices);
}

TEST_F(MathTest, triangulate_simple_convex_poly)
{
  std::vector<Vec4f> vertices{
    { 0, 0, 0, 1 },
    { 1, 0, 0, 1 },
    { 1, 0, 1, 1 },
    { 0.8f, 0, 1.2f, 1 },
    { 0, 0, 1, 1 }
  };

  auto indices = triangulatePoly(vertices);
  std::vector<uint16_t> expected{ 0, 1, 2, 0, 2, 3, 0, 3, 4 };

  ASSERT_EQ(expected, indices);
}

TEST_F(MathTest, triangulate_nonconvex_poly)
{
  std::vector<Vec4f> vertices{
    { 0, 0, 0, 1 },
    { 1, 0, 0, 1 },
    { 1, 0, 1, 1 },
    { 0.8f, 0, 0.5f, 1 },
    { 0, 0, 1, 1 }
  };

  auto indices = triangulatePoly(vertices);
  std::vector<uint16_t> expected{ 1, 2, 3, 0, 1, 3, 0, 3, 4 };

  ASSERT_EQ(expected, indices);
}
