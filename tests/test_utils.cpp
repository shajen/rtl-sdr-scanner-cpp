#include <gtest/gtest.h>
#include <utils.h>

class UtilsTest : public ::testing::Test {
 public:
  std::vector<FrequencyRange> calculate(Frequency start, Frequency stop, Frequency sampleRate, uint32_t fft) { return fitFrequencyRange({start, stop, sampleRate, fft}); }
  FrequencyRange range(Frequency start, Frequency stop, Frequency sampleRate, uint32_t fft) { return {start, stop, sampleRate, fft}; }
};

TEST_F(UtilsTest, RtlSdrSingleRangeTest) {
  const std::vector<FrequencyRange> results{range(144000000, 146000000, 2048000, 16384)};
  EXPECT_EQ(results, calculate(144000000, 146000000, 2048000, 16384));
}

TEST_F(UtilsTest, RtlSdrSplitRangeTest) {
  const std::vector<FrequencyRange> results{range(144000000, 146000000, 2048000, 16384), range(146000000, 148000000, 2048000, 16384)};
  EXPECT_EQ(results, calculate(144000000, 148000000, 2048000, 16384));
}

TEST_F(UtilsTest, HackRfSingleRangeTest) {
  const std::vector<FrequencyRange> results{range(130000000, 150000000, 20480000, 16384)};
  EXPECT_EQ(results, calculate(130000000, 150000000, 20480000, 16384));
}

TEST_F(UtilsTest, HackRfSplitRangeTest) {
  const std::vector<FrequencyRange> results{range(130000000, 150000000, 20480000, 16384), range(150000000, 170000000, 20480000, 16384)};
  EXPECT_EQ(results, calculate(130000000, 170000000, 20480000, 16384));
}

TEST_F(UtilsTest, Fft) {
  EXPECT_EQ(countFft(2048000), 2048);
  EXPECT_EQ(countFft(20480000), 16384);
}
