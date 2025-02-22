#include <triple_buffer.hpp>
#include <gtest/gtest.h>

class TripleBufferTest : public testing::Test
{
  public:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

TEST_F(TripleBufferTest, writeComplete_should_not_clobber_readable)
{
  struct Data
  {
    int value = 0;
  };

  TripleBuffer<Data> buffer;

  for (int i = 0; i < 10; ++i) {
    buffer.getWritable().value = 123;
    buffer.writeComplete();

    ASSERT_EQ(0, buffer.getReadable().value);
  }
}

TEST_F(TripleBufferTest, can_read_latest_written_value)
{
  struct Data
  {
    int value = 0;
  };

  TripleBuffer<Data> buffer;

  buffer.getWritable().value = 123;
  buffer.writeComplete();
  buffer.getWritable().value = 234;
  buffer.writeComplete();

  ASSERT_EQ(0, buffer.getReadable().value);
  buffer.readComplete();

  ASSERT_EQ(234, buffer.getReadable().value);
}
