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
  std::map<int, bool> indexes({{10, false}, {14, false}});

  EXPECT_FALSE(containsWithMargin(indexes, 9, 0));
  EXPECT_TRUE(containsWithMargin(indexes, 10, 0));
  EXPECT_FALSE(containsWithMargin(indexes, 11, 0));

  EXPECT_FALSE(containsWithMargin(indexes, 13, 0));
  EXPECT_TRUE(containsWithMargin(indexes, 14, 0));
  EXPECT_FALSE(containsWithMargin(indexes, 15, 0));
}

TEST(Utils, ContaisWithMargin1) {
  std::map<int, bool> indexes({{10, false}, {14, false}});

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
  std::map<int, bool> indexes({{10, false}, {14, false}});

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

TEST(Utils, AverageX) {
  std::vector<float> input({1, 2, 3, 4, 5, 6, 7, 8, 9});
  std::vector<float> output(input.size(), 0.0);
  std::vector<float> result({2, 2.5, 3, 4, 5, 6, 7, 7.5, 8});

  average(input.data(), output.data(), input.size(), 5);
  for (size_t i = 0; i < result.size(); ++i) {
    EXPECT_FLOAT_EQ(output[i], result[i]);
  }
}

TEST(Utils, mostFrequentValue) {
  std::vector<int> v1({1, 2, 3, 4, 5, 5});
  EXPECT_EQ(mostFrequentValue(v1), 5);

  std::vector<int> v2({3, 3, 1, 1, 5, 5});
  EXPECT_EQ(mostFrequentValue(v2), 3);

  std::vector<int> v3({3, 3, 1, 1, 5, 5, 2, 2});
  EXPECT_EQ(mostFrequentValue(v3), 3);

  std::vector<int> v4({1, 1, 1, 1, 2, 5, 5, 5});
  EXPECT_EQ(mostFrequentValue(v4), 1);

  std::vector<int> v5({1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10});
  EXPECT_EQ(mostFrequentValue(v5), 1);
}

TEST(Utils, getNearestElement) {
  std::set<int> data({10, 20, 30, 40});

  EXPECT_EQ(getNearestElement(data, 9), 10);
  EXPECT_EQ(getNearestElement(data, 10), 10);
  EXPECT_EQ(getNearestElement(data, 11), 10);

  EXPECT_EQ(getNearestElement(data, 24), 20);
  EXPECT_EQ(getNearestElement(data, 25), 30);
  EXPECT_EQ(getNearestElement(data, 26), 30);

  EXPECT_EQ(getNearestElement(data, 39), 40);
  EXPECT_EQ(getNearestElement(data, 40), 40);
  EXPECT_EQ(getNearestElement(data, 41), 40);
}

TEST(Utils, getMaxIndex) {
  std::vector<float> data({1, 2, 3, 4, 5, 4, 3, 2, 1});
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 0, 0), 0);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 0, 1), 0);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 0, 2), 1);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 0, 3), 1);

  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 8, 0), 8);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 8, 1), 8);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 8, 2), 7);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 8, 3), 7);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 8, 4), 6);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 8, 5), 6);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 8, 6), 5);

  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 0), 2);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 1), 2);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 2), 3);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 3), 3);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 4), 4);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 5), 4);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 6), 4);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 7), 4);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 8), 4);
  EXPECT_EQ(getMaxIndex(data.data(), data.size(), 2, 9), 4);
}