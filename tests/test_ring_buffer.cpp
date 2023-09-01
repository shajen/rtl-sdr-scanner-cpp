#include <gtest/gtest.h>
#include <ring_buffer.h>

TEST(RingBufferTest, Empty) {
  RingBuffer buffer(100);
  std::vector<uint8_t> tmp(20);

  EXPECT_EQ(buffer.availableDataSize(), 0);
  EXPECT_EQ(buffer.availableSpaceSize(), 100);

  buffer.push(tmp.data(), tmp.size());
  EXPECT_EQ(buffer.availableDataSize(), 20);
  EXPECT_EQ(buffer.availableSpaceSize(), 80);

  buffer.clear();
  EXPECT_EQ(buffer.availableDataSize(), 0);
  EXPECT_EQ(buffer.availableSpaceSize(), 100);
}

TEST(RingBufferTest, Overflow) {
  constexpr auto SIZE = 31;
  constexpr auto ITERATION = 83;

  RingBuffer buffer(ITERATION * SIZE - 1);
  std::vector<uint8_t> tmp(SIZE);
  for (int i = 1; i < ITERATION; ++i) {
    buffer.push(tmp.data(), tmp.size());
    EXPECT_EQ(buffer.availableDataSize(), i * SIZE);
  }
  for (int i = 0; i < ITERATION / 2; ++i) {
    buffer.pop(SIZE);
    buffer.push(tmp.data(), tmp.size());
  }
  EXPECT_EQ(buffer.availableSpaceSize(), SIZE - 1);
  buffer.push(tmp.data(), tmp.size());
  EXPECT_EQ(buffer.availableDataSize(), 0);
}

TEST(RingBufferTest, Round) {
  std::vector<uint8_t> push;
  std::vector<uint8_t> popped;

  constexpr auto PUSH_SIZE = 1229;
  constexpr auto POP_SIZE = 1231;
  constexpr auto BUFFER_SIZE = 1277 * 10;

  for (int i = 0; i < 100 * PUSH_SIZE * POP_SIZE; ++i) {
    push.push_back(rand());
  }

  RingBuffer buffer(BUFFER_SIZE);
  uint32_t pushed = 0;
  while (popped.size() < push.size()) {
    while (buffer.availableDataSize() < POP_SIZE && pushed < push.size()) {
      buffer.push(push.data() + pushed, PUSH_SIZE);
      pushed += PUSH_SIZE;
    }
    while (POP_SIZE <= buffer.availableDataSize()) {
      const auto tmp = buffer.pop(POP_SIZE);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(popped));
    }
  }

  EXPECT_EQ(push, popped);
}
