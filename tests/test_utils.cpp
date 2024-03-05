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
