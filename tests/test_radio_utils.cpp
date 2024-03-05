#include <gtest/gtest.h>
#include <utils/radio_utils.h>

TEST(RadioUtils, Fft) {
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

TEST(RadioUtils, PrimeFactors) {
  EXPECT_EQ(getPrimeFactors(1), std::vector<int>({1}));
  EXPECT_EQ(getPrimeFactors(2), std::vector<int>({2}));
  EXPECT_EQ(getPrimeFactors(3), std::vector<int>({3}));
  EXPECT_EQ(getPrimeFactors(4), std::vector<int>({2, 2}));
  EXPECT_EQ(getPrimeFactors(89), std::vector<int>({89}));
  EXPECT_EQ(getPrimeFactors(1250), std::vector<int>({2, 5, 5, 5, 5}));
  EXPECT_EQ(getPrimeFactors(1200500), std::vector<int>({2, 2, 5, 5, 5, 7, 7, 7, 7}));
}

TEST(RadioUtils, ResamplersRandom) {
  using Result = std::vector<std::pair<int, int>>;
  const auto threshold = 125;

  EXPECT_EQ(getResamplersFactors(1, 1, threshold), Result({{1, 1}}));
  EXPECT_EQ(getResamplersFactors(7823, 7823, threshold), Result({{1, 1}}));
  EXPECT_EQ(getResamplersFactors(7823, 7883, threshold), Result({{7883, 7823}}));
}

TEST(RadioUtils, ResamplersTypical16kHz) {
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

TEST(RadioUtils, ResamplersTypical20kHz) {
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

TEST(RadioUtils, TunedFrequency) {
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

TEST(RadioUtils, RangeSplitSampleRate) {
  EXPECT_EQ(getRangeSplitSampleRate(81920000), 81000000);
  EXPECT_EQ(getRangeSplitSampleRate(80000000), 80000000);
  EXPECT_EQ(getRangeSplitSampleRate(40960000), 40000000);
  EXPECT_EQ(getRangeSplitSampleRate(40000000), 40000000);
  EXPECT_EQ(getRangeSplitSampleRate(20480000), 20000000);
  EXPECT_EQ(getRangeSplitSampleRate(20000000), 20000000);
  EXPECT_EQ(getRangeSplitSampleRate(10240000), 10000000);
  EXPECT_EQ(getRangeSplitSampleRate(10000000), 10000000);
  EXPECT_EQ(getRangeSplitSampleRate(3200000), 3000000);
  EXPECT_EQ(getRangeSplitSampleRate(2880000), 2500000);
  EXPECT_EQ(getRangeSplitSampleRate(2560000), 2500000);
  EXPECT_EQ(getRangeSplitSampleRate(2160000), 2000000);
  EXPECT_EQ(getRangeSplitSampleRate(2048000), 2000000);
  EXPECT_EQ(getRangeSplitSampleRate(1920000), 1500000);
  EXPECT_EQ(getRangeSplitSampleRate(1720000), 1500000);
  EXPECT_EQ(getRangeSplitSampleRate(1024000), 1000000);
  EXPECT_EQ(getRangeSplitSampleRate(250000), 200000);
}

TEST(RadioUtils, SplitRanges) {
  using Ranges = std::vector<FrequencyRange>;
  EXPECT_EQ(splitRange({140000000, 160000000}, 20000000), Ranges({{140000000, 160000000}}));
  EXPECT_EQ(splitRange({140000000, 180000000}, 20000000), Ranges({{140000000, 160000000}, {160000000, 180000000}}));
  EXPECT_EQ(splitRange({140000000, 200000000}, 20000000), Ranges({{140000000, 160000000}, {160000000, 180000000}, {180000000, 200000000}}));
  EXPECT_EQ(splitRange({140000000, 145000000}, 2000000), Ranges({{140000000, 142000000}, {142000000, 144000000}, {144000000, 146000000}}));
  EXPECT_EQ(splitRange({140000000, 150000000}, 2000000), Ranges({{140000000, 142000000}, {142000000, 144000000}, {144000000, 146000000}, {146000000, 148000000}, {148000000, 150000000}}));
}
