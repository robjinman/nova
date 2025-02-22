#include <tree_set.hpp>
#include <gtest/gtest.h>
#include <vector>

class TreeSetTest : public testing::Test
{
  public:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

TEST_F(TreeSetTest, iterate_over_empty_set)
{
  TreeSet<int, char> set;

  for ([[maybe_unused]] auto& element : set) {
  }
}

TEST_F(TreeSetTest, iterates_in_correct_order)
{
  TreeSet<int, char> set;

  set.insert({ 3, 5, 2 }, 'A');
  set.insert({ 2, 1, 4 }, 'B');
  set.insert({ 3, 5, 6 }, 'C');
  set.insert({ 3, 7 }, 'D');
  set.insert({ 3, 5, 3, 1 }, 'E');

  std::vector<char> expected{ 'B', 'A', 'E', 'C', 'D' };
  std::vector<char> v;

  for (auto& element : set) {
    v.push_back(element);
  }

  ASSERT_EQ(expected, v);
}

TEST_F(TreeSetTest, find_element)
{
  TreeSet<int, char> set;

  set.insert({ 3, 5, 2 }, 'A');
  set.insert({ 2, 1, 4 }, 'B');
  set.insert({ 3, 5, 6 }, 'C');
  set.insert({ 3, 7 }, 'D');
  set.insert({ 3, 5, 3, 1 }, 'E');

  auto i = set.find({ 3, 5, 3, 1 });

  ASSERT_EQ('E', *i);
  ASSERT_EQ('C', *(++i));
  ASSERT_EQ('D', *(++i));
}

TEST_F(TreeSetTest, find_nonexistent_element_returns_end_iterator)
{
  TreeSet<int, char> set;

  set.insert({ 3, 5, 2 }, 'A');
  set.insert({ 2, 1, 4 }, 'B');
  set.insert({ 3, 5, 6 }, 'C');
  set.insert({ 3, 7 }, 'D');
  set.insert({ 3, 5, 3, 1 }, 'E');

  auto i = set.find({ 3, 2, 3, 1 });

  ASSERT_EQ(i, set.end());
}

TEST_F(TreeSetTest, find_non_leaf_element_returns_end_iterator)
{
  TreeSet<int, char> set;

  set.insert({ 3, 5, 2 }, 'A');
  set.insert({ 2, 1, 4 }, 'B');
  set.insert({ 3, 5, 6 }, 'C');
  set.insert({ 3, 7 }, 'D');
  set.insert({ 3, 5, 3, 1 }, 'E');

  auto i = set.find({ 3, 5 });

  ASSERT_EQ(i, set.end());
}
