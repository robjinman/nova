#include <sorted_set.hpp>
#include <gtest/gtest.h>
#include <vector>

class SortedSetTest : public testing::Test
{
  public:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

TEST_F(SortedSetTest, iterates_in_correct_order)
{
  SortedSet<int, char> set;

  set.add({ 3, 5, 2 }, 'A');
  set.add({ 2, 1, 4 }, 'B');
  set.add({ 3, 5, 6 }, 'C');
  set.add({ 3, 7 }, 'D');
  set.add({ 3, 5, 3, 1 }, 'E');

  std::vector<char> expected{ 'B', 'A', 'E', 'C', 'D' };
  std::vector<char> v;

  for (auto& element : set) {
    v.push_back(element);
  }

  ASSERT_EQ(expected, v);
}

TEST_F(SortedSetTest, find_element)
{
  SortedSet<int, char> set;

  set.add({ 3, 5, 2 }, 'A');
  set.add({ 2, 1, 4 }, 'B');
  set.add({ 3, 5, 6 }, 'C');
  set.add({ 3, 7 }, 'D');
  set.add({ 3, 5, 3, 1 }, 'E');

  auto i = set.find({ 3, 5, 3, 1 });

  ASSERT_EQ('E', *i);
  ASSERT_EQ('C', *(++i));
  ASSERT_EQ('D', *(++i));
}

TEST_F(SortedSetTest, find_nonexistent_element_returns_end_iterator)
{
  SortedSet<int, char> set;

  set.add({ 3, 5, 2 }, 'A');
  set.add({ 2, 1, 4 }, 'B');
  set.add({ 3, 5, 6 }, 'C');
  set.add({ 3, 7 }, 'D');
  set.add({ 3, 5, 3, 1 }, 'E');

  auto i = set.find({ 3, 2, 3, 1 });

  ASSERT_EQ(i, set.end());
}

TEST_F(SortedSetTest, find_non_leaf_element_returns_end_iterator)
{
  SortedSet<int, char> set;

  set.add({ 3, 5, 2 }, 'A');
  set.add({ 2, 1, 4 }, 'B');
  set.add({ 3, 5, 6 }, 'C');
  set.add({ 3, 7 }, 'D');
  set.add({ 3, 5, 3, 1 }, 'E');

  auto i = set.find({ 3, 5 });

  ASSERT_EQ(i, set.end());
}
