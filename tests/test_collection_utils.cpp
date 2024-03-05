#include <gtest/gtest.h>
#include <utils/collection_utils.h>

TEST(CollectionUtils, ContaisWithMargin0) {
  std::map<int, bool> indexes({{10, false}, {14, false}});

  EXPECT_FALSE(containsWithMargin(indexes, 9, 0));
  EXPECT_TRUE(containsWithMargin(indexes, 10, 0));
  EXPECT_FALSE(containsWithMargin(indexes, 11, 0));

  EXPECT_FALSE(containsWithMargin(indexes, 13, 0));
  EXPECT_TRUE(containsWithMargin(indexes, 14, 0));
  EXPECT_FALSE(containsWithMargin(indexes, 15, 0));
}

TEST(CollectionUtils, ContaisWithMargin1) {
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

TEST(CollectionUtils, ContaisWithMargin2) {
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

TEST(CollectionUtils, mostFrequentValue) {
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

TEST(CollectionUtils, getNearestElement) {
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

TEST(CollectionUtils, getMaxIndex) {
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