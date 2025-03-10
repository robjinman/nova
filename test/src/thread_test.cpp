#include <thread.hpp>
#include <gtest/gtest.h>

class ThreadTest : public testing::Test
{
  public:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

TEST_F(ThreadTest, wait_for_task)
{
  Thread thread;
  int result = 0;
  thread.run<void>([&]() {
    for (int i = 0; i < 100; ++i) {
      result += i;
    }
  }).wait();

  ASSERT_EQ(4950, result);
}

TEST_F(ThreadTest, wait_for_int_result)
{
  Thread thread;
  std::future<int> future = thread.run<int>([&]() {
    int result = 0;
    for (int i = 0; i < 100; ++i) {
      result += i;
    }
    return result;
  });
  int result = future.get();

  ASSERT_EQ(4950, result);
}

TEST_F(ThreadTest, exception_on_get)
{
  Thread thread;
  std::future<int> future = thread.run<int>([&]() -> int {
    int result = 0;
    for (int i = 0; i < 100; ++i) {
      result += i;
    }
    if (result > 1234) {
      throw std::runtime_error("Error!");
    }
    return result;
  });

  EXPECT_THROW({
    try {
      future.get();
    }
    catch (const std::exception& e) {
      EXPECT_EQ(std::string("Error!"), e.what());
      throw e;
    }
  }, std::exception);
}
