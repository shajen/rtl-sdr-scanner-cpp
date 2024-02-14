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

TEST(Utils, PrimeFactors) {
  EXPECT_EQ(getPrimeFactors(1), std::vector<int>({1}));
  EXPECT_EQ(getPrimeFactors(2), std::vector<int>({2}));
  EXPECT_EQ(getPrimeFactors(3), std::vector<int>({3}));
  EXPECT_EQ(getPrimeFactors(4), std::vector<int>({2, 2}));
  EXPECT_EQ(getPrimeFactors(89), std::vector<int>({89}));
  EXPECT_EQ(getPrimeFactors(1250), std::vector<int>({2, 5, 5, 5, 5}));
  EXPECT_EQ(getPrimeFactors(1200500), std::vector<int>({2, 2, 5, 5, 5, 7, 7, 7, 7}));
}

TEST(Utils, ResamplersRandom) {
  using Result = std::vector<std::pair<int, int>>;
  const auto threshold = 125;

  EXPECT_EQ(getResamplersFactors(1, 1, threshold), Result({{1, 1}}));
  EXPECT_EQ(getResamplersFactors(7823, 7823, threshold), Result({{1, 1}}));
  EXPECT_EQ(getResamplersFactors(7823, 7883, threshold), Result({{7883, 7823}}));
}

TEST(Utils, ResamplersTypical16kHz) {
  using Result = std::vector<std::pair<int, int>>;
  const auto threshold = 125;

  EXPECT_EQ(getResamplersFactors(1000000, 16000, threshold), Result({{2, 125}}));
  EXPECT_EQ(getResamplersFactors(10000000, 16000, threshold), Result({{1, 25}, {1, 25}}));

  EXPECT_EQ(getResamplersFactors(1024000, 16000, threshold), Result({{1, 64}}));
  EXPECT_EQ(getResamplersFactors(10240000, 16000, threshold), Result({{1, 20}, {1, 32}}));

  EXPECT_EQ(getResamplersFactors(2000000, 16000, threshold), Result({{1, 125}}));
  EXPECT_EQ(getResamplersFactors(20000000, 16000, threshold), Result({{1, 25}, {1, 50}}));

  EXPECT_EQ(getResamplersFactors(2048000, 16000, threshold), Result({{1, 8}, {1, 16}}));
  EXPECT_EQ(getResamplersFactors(20480000, 16000, threshold), Result({{1, 32}, {1, 40}}));
}

TEST(Utils, ResamplersTypical20kHz) {
  using Result = std::vector<std::pair<int, int>>;
  const auto threshold = 125;

  EXPECT_EQ(getResamplersFactors(1000000, 20000, threshold), Result({{1, 50}}));
  EXPECT_EQ(getResamplersFactors(10000000, 20000, threshold), Result({{1, 20}, {1, 25}}));

  EXPECT_EQ(getResamplersFactors(1024000, 20000, threshold), Result({{1, 16}, {5, 16}}));
  EXPECT_EQ(getResamplersFactors(10240000, 20000, threshold), Result({{1, 16}, {1, 32}}));

  EXPECT_EQ(getResamplersFactors(2000000, 20000, threshold), Result({{1, 100}}));
  EXPECT_EQ(getResamplersFactors(20000000, 20000, threshold), Result({{1, 25}, {1, 40}}));

  EXPECT_EQ(getResamplersFactors(2048000, 20000, threshold), Result({{1, 16}, {5, 32}}));
  EXPECT_EQ(getResamplersFactors(20480000, 20000, threshold), Result({{1, 32}, {1, 32}}));
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
