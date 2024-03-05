#include <gtest/gtest.h>
#include <utils/utils.h>

TEST(Utils, AverageX) {
  std::vector<float> input({1, 2, 3, 4, 5, 6, 7, 8, 9});
  std::vector<float> output(input.size(), 0.0);
  std::vector<float> result({2, 2.5, 3, 4, 5, 6, 7, 7.5, 8});

  average(input.data(), output.data(), input.size(), 5);
  for (size_t i = 0; i < result.size(); ++i) {
    EXPECT_FLOAT_EQ(output[i], result[i]);
  }
}

TEST(Utils, RoundUp) {
  EXPECT_FLOAT_EQ(roundUp(19999999, 1000000), 20000000);
  EXPECT_FLOAT_EQ(roundUp(20000000, 1000000), 20000000);
  EXPECT_FLOAT_EQ(roundUp(20000001, 1000000), 21000000);
}

TEST(Utils, RoundDown) {
  EXPECT_FLOAT_EQ(roundDown(19999999, 1000000), 19000000);
  EXPECT_FLOAT_EQ(roundDown(20000000, 1000000), 20000000);
  EXPECT_FLOAT_EQ(roundDown(20000001, 1000000), 20000000);
}
