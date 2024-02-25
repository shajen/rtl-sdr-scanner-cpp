#include <gtest/gtest.h>
#include <radio/averager.h>
#include <utils.h>

TEST(Averager, SimpleTest) {
  const int size = 5;
  std::vector<float> data;
  std::vector<float> result;
  auto f = [size](const float value) { return std::vector<float>(size, value); };
  Averager avg(size, 3);

  EXPECT_EQ(avg.average(), f(-100));

  avg.push(f(1).data());
  EXPECT_EQ(avg.average(), f(-100));

  avg.push(f(2).data());
  EXPECT_EQ(avg.average(), f(-100));

  avg.push(f(3).data());
  EXPECT_EQ(avg.average(), f(2));

  avg.push(f(10).data());
  EXPECT_EQ(avg.average(), f(5));

  avg.push(f(11).data());
  EXPECT_EQ(avg.average(), f(8));

  avg.reset();
  EXPECT_EQ(avg.average(), f(-100));

  avg.push(f(1).data());
  EXPECT_EQ(avg.average(), f(-100));

  avg.push(f(2).data());
  EXPECT_EQ(avg.average(), f(-100));

  avg.push(f(3).data());
  EXPECT_EQ(avg.average(), f(2));

  avg.push(f(10).data());
  EXPECT_EQ(avg.average(), f(5));

  avg.push(f(11).data());
  EXPECT_EQ(avg.average(), f(8));
}