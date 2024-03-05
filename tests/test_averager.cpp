#include <gtest/gtest.h>
#include <radio/averager.h>
#include <utils/utils.h>

#include <deque>

constexpr auto SIZE = 5;
constexpr auto GROUP_SIZE = 3;

std::vector<float> generate(const float value) { return std::vector<float>(SIZE, value); }
std::deque<std::vector<float>> generateRaw(const float v1, const float v2, const float v3) { return {generate(v1), generate(v2), generate(v3)}; }

class AveragerTest : public testing::Test {
 public:
  AveragerTest() : m_averager(SIZE, GROUP_SIZE) {
    while (m_rawData.size() < GROUP_SIZE) {
      m_rawData.emplace_back(SIZE, 0.0);
    }
  }

  void add(const std::vector<float>& data) {
    m_averager.push(data.data());
    m_rawData.push_back(data);
    if (GROUP_SIZE < m_rawData.size()) {
      m_rawData.pop_front();
    }
  }

  std::vector<float> average() {
    std::vector<float> sum(SIZE, 0.0);
    for (const auto& data : m_rawData) {
      for (size_t i = 0; i < data.size(); ++i) {
        sum[i] += data[i];
      }
    }
    for (auto& value : sum) {
      value = value / GROUP_SIZE;
    }
    return sum;
  }

  Averager m_averager;
  std::deque<std::vector<float>> m_rawData;
};

TEST_F(AveragerTest, SimpleTest) {
  add({1, 2, 3, 4, 5});
  EXPECT_EQ(m_averager.average(), generate(-100));
  EXPECT_EQ(m_averager.data(), m_rawData);

  add({2, 3, 4, 5, 6});
  EXPECT_EQ(m_averager.average(), generate(-100));
  EXPECT_EQ(m_averager.data(), m_rawData);

  add({3, 4, 5, 6, 7});
  EXPECT_EQ(m_averager.average(), average());
  EXPECT_EQ(m_averager.data(), m_rawData);

  add({6, 7, 8, 9, 10});
  EXPECT_EQ(m_averager.average(), average());
  EXPECT_EQ(m_averager.data(), m_rawData);

  add({7, 8, 9, 10, 11});
  EXPECT_EQ(m_averager.average(), average());
  EXPECT_EQ(m_averager.data(), m_rawData);
}

TEST_F(AveragerTest, SimpleBigTest) {
  add({1, 2, 3, 4, 5});
  EXPECT_EQ(m_averager.average(), generate(-100));
  EXPECT_EQ(m_averager.data(), m_rawData);

  add({2, 3, 4, 5, 6});
  EXPECT_EQ(m_averager.average(), generate(-100));
  EXPECT_EQ(m_averager.data(), m_rawData);

  for (int i = 1; i < 123; ++i) {
    std::vector<float> data;
    for (int j = 0; j < SIZE; ++j) {
      data.push_back(i * 11 + j * 7);
    }
    add(data);
    EXPECT_EQ(m_averager.average(), average());
    EXPECT_EQ(m_averager.data(), m_rawData);
  }
}

TEST(Averager, SimpleTest) {
  const int size = 5;
  std::vector<float> data;
  std::vector<float> result;
  Averager avg(size, 3);

  EXPECT_EQ(avg.average(), generate(-100));
  EXPECT_EQ(avg.data(), generateRaw(0, 0, 0));

  avg.push(generate(1).data());
  EXPECT_EQ(avg.average(), generate(-100));
  EXPECT_EQ(avg.data(), generateRaw(0, 0, 1));

  avg.push(generate(2).data());
  EXPECT_EQ(avg.average(), generate(-100));
  EXPECT_EQ(avg.data(), generateRaw(0, 1, 2));

  avg.push(generate(3).data());
  EXPECT_EQ(avg.average(), generate(2));
  EXPECT_EQ(avg.data(), generateRaw(1, 2, 3));

  avg.push(generate(10).data());
  EXPECT_EQ(avg.average(), generate(5));
  EXPECT_EQ(avg.data(), generateRaw(2, 3, 10));

  avg.push(generate(11).data());
  EXPECT_EQ(avg.average(), generate(8));
  EXPECT_EQ(avg.data(), generateRaw(3, 10, 11));

  avg.reset();
  EXPECT_EQ(avg.average(), generate(-100));
  EXPECT_EQ(avg.data(), generateRaw(0, 0, 0));

  avg.push(generate(1).data());
  EXPECT_EQ(avg.average(), generate(-100));
  EXPECT_EQ(avg.data(), generateRaw(0, 0, 1));

  avg.push(generate(2).data());
  EXPECT_EQ(avg.average(), generate(-100));
  EXPECT_EQ(avg.data(), generateRaw(0, 1, 2));

  avg.push(generate(3).data());
  EXPECT_EQ(avg.average(), generate(2));
  EXPECT_EQ(avg.data(), generateRaw(1, 2, 3));

  avg.push(generate(10).data());
  EXPECT_EQ(avg.average(), generate(5));
  EXPECT_EQ(avg.data(), generateRaw(2, 3, 10));

  avg.push(generate(11).data());
  EXPECT_EQ(avg.average(), generate(8));
  EXPECT_EQ(avg.data(), generateRaw(3, 10, 11));
}
