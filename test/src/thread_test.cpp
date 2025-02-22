#include <thread.hpp>
#include <gtest/gtest.h>

class ThreadTest : public testing::Test
{
  public:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

TEST_F(ThreadTest, wait_doesnt_block_when_no_task)
{
  Thread thread;
  thread.wait();
}

TEST_F(ThreadTest, wait_for_task)
{
  Thread thread;
  int result = 0;
  thread.run([&]() {
    for (int i = 0; i < 100; ++i) {
      result += i;
    }
  });
  thread.wait();

  ASSERT_EQ(4950, result);
}
