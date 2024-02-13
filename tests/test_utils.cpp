#include <gtest/gtest.h>
#include <utils.h>

TEST(Utils, Fft) {
  EXPECT_EQ(getFft(2048000 - 1, 1000), 2048);
  EXPECT_EQ(getFft(2048000, 1000), 2048);
  EXPECT_EQ(getFft(2048000 + 1, 1000), 4096);

  EXPECT_EQ(getFft(20480000 - 1, 625), 32768);
  EXPECT_EQ(getFft(20480000, 625), 32768);
  EXPECT_EQ(getFft(20480000 + 1, 625), 65536);

  EXPECT_EQ(getFft(104857600 - 1, 100), 1048576);
  EXPECT_EQ(getFft(104857600, 100), 1048576);
  EXPECT_EQ(getFft(104857600 + 1, 100), 2097152);
}

TEST(Utils, Resampler) {
  auto result = [](int a, int b) { return std::make_pair(a, b); };
  EXPECT_EQ(getResamplerFactors(20000, 16000), result(4, 5));
  EXPECT_EQ(getResamplerFactors(2048000, 16000), result(1, 128));
  EXPECT_EQ(getResamplerFactors(20480000, 16000), result(1, 1280));
}

TEST(Utils, TunedFrequency) {
  EXPECT_EQ(getTunedFrequency(-999, 1000), -1000);
  EXPECT_EQ(getTunedFrequency(-1001, 1000), -1000);
  EXPECT_EQ(getTunedFrequency(-1001, 1000), -1000);

  EXPECT_EQ(getTunedFrequency(-1499, 1000), -1000);
  EXPECT_EQ(getTunedFrequency(-1500, 1000), -1000);
  EXPECT_EQ(getTunedFrequency(-1501, 1000), -2000);

  EXPECT_EQ(getTunedFrequency(999, 1000), 1000);
  EXPECT_EQ(getTunedFrequency(1001, 1000), 1000);
  EXPECT_EQ(getTunedFrequency(1001, 1000), 1000);

  EXPECT_EQ(getTunedFrequency(1499, 1000), 1000);
  EXPECT_EQ(getTunedFrequency(1500, 1000), 2000);
  EXPECT_EQ(getTunedFrequency(1501, 1000), 2000);
}

TEST(Utils, ContaisWithMargin0) {
  std::set<int> indexes({10, 14});

  EXPECT_FALSE(containsWithMargin(indexes, 9, 0));
  EXPECT_TRUE(containsWithMargin(indexes, 10, 0));
  EXPECT_FALSE(containsWithMargin(indexes, 11, 0));

  EXPECT_FALSE(containsWithMargin(indexes, 13, 0));
  EXPECT_TRUE(containsWithMargin(indexes, 14, 0));
  EXPECT_FALSE(containsWithMargin(indexes, 15, 0));
}

TEST(Utils, ContaisWithMargin1) {
  std::set<int> indexes({10, 14});

  EXPECT_FALSE(containsWithMargin(indexes, 8, 1));
  EXPECT_TRUE(containsWithMargin(indexes, 9, 1));
  EXPECT_TRUE(containsWithMargin(indexes, 10, 1));
  EXPECT_TRUE(containsWithMargin(indexes, 11, 1));
  EXPECT_FALSE(containsWithMargin(indexes, 12, 1));

  EXPECT_FALSE(containsWithMargin(indexes, 12, 1));
  EXPECT_TRUE(containsWithMargin(indexes, 13, 1));
  EXPECT_TRUE(containsWithMargin(indexes, 14, 1));
  EXPECT_TRUE(containsWithMargin(indexes, 15, 1));
  EXPECT_FALSE(containsWithMargin(indexes, 16, 1));
}

TEST(Utils, ContaisWithMargin2) {
  std::set<int> indexes({10, 14});

  EXPECT_FALSE(containsWithMargin(indexes, 8, 2));
  EXPECT_TRUE(containsWithMargin(indexes, 9, 2));
  EXPECT_TRUE(containsWithMargin(indexes, 10, 2));
  EXPECT_TRUE(containsWithMargin(indexes, 11, 2));
  EXPECT_FALSE(containsWithMargin(indexes, 12, 2));

  EXPECT_FALSE(containsWithMargin(indexes, 12, 2));
  EXPECT_TRUE(containsWithMargin(indexes, 13, 2));
  EXPECT_TRUE(containsWithMargin(indexes, 14, 2));
  EXPECT_TRUE(containsWithMargin(indexes, 15, 2));
  EXPECT_FALSE(containsWithMargin(indexes, 16, 2));
}