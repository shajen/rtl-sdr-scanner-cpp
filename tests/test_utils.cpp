#include <gtest/gtest.h>
#include <utils.h>

class UtilsTest : public ::testing::Test {
 public:
  std::vector<FrequencyRange> calculate(Frequency start, Frequency stop, Frequency step, Frequency sampleRate) { return fitFrequencyRange({start, stop, step, sampleRate}); }
  FrequencyRange range(Frequency start, Frequency stop, Frequency step, Frequency sampleRate) { return {start, stop, step, sampleRate}; }
};

TEST_F(UtilsTest, RtlSdrSingleRangeTest) {
  const std::vector<FrequencyRange> results{range(144000000, 146000000, 125, 2048000)};
  EXPECT_EQ(results, calculate(144000000, 146000000, 125, 2048000));
}

TEST_F(UtilsTest, RtlSdrSplitRangeTest) {
  const std::vector<FrequencyRange> results{range(144000000, 146000000, 125, 2048000), range(146000000, 148000000, 125, 2048000)};
  EXPECT_EQ(results, calculate(144000000, 148000000, 125, 2048000));
}

TEST_F(UtilsTest, RtlSdrInvalidFftSize) { EXPECT_THROW(calculate(144000000, 148000000, 100, 2048000), std::runtime_error); }

TEST_F(UtilsTest, HackRfSingleRangeTest) {
  const std::vector<FrequencyRange> results{range(130000000, 150000000, 1250, 20480000)};
  EXPECT_EQ(results, calculate(130000000, 150000000, 1250, 20480000));
}

TEST_F(UtilsTest, HackRfSplitRangeTest) {
  const std::vector<FrequencyRange> results{range(130000000, 150000000, 1250, 20480000), range(150000000, 170000000, 1250, 20480000)};
  EXPECT_EQ(results, calculate(130000000, 170000000, 1250, 20480000));
}

TEST_F(UtilsTest, HackRfInvalidFftSize) { EXPECT_THROW(calculate(140000000, 150000000, 250, 20480000), std::runtime_error); }
