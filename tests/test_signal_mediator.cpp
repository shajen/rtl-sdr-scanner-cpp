#include <algorithms/signal_mediator.h>
#include <gtest/gtest.h>

std::vector<Signal> genSignals(const Frequency& start, const Frequency& stop, const Frequency& step, const Power power) {
  std::vector<Signal> signals;
  for (Frequency f = start; f <= stop; f += step) {
    signals.push_back({f, power});
  }
  return signals;
}

void test(
    const Frequency start,
    const Frequency stop,
    const Frequency step,
    const std::chrono::milliseconds aggTime,
    const std::chrono::milliseconds startTime,
    const std::chrono::milliseconds duration,
    const std::chrono::milliseconds stepTime) {
  const auto signalsSize = (stop - start) / step + 1;
  SignalMediator sm(aggTime);
  for (std::chrono::milliseconds t{0}; t < duration; t += stepTime) {
    const auto power = static_cast<Power>(-(t.count() % 100));
    const auto result = sm.append(startTime + t, genSignals(start, stop, step, power));
    if (t % aggTime != std::chrono::milliseconds(0) || t == std::chrono::milliseconds(0)) {
      EXPECT_EQ(result.size(), 0);
    } else {
      EXPECT_EQ(result.size(), signalsSize);
    }
  }
}

TEST(SignalMediatorTest, test1) {
  const Frequency start = 144000000;
  const Frequency stop = 146000000;
  const Frequency step = 1000;
  const auto aggTime = std::chrono::milliseconds(1);
  const auto startTime = std::chrono::milliseconds(123456789);
  const auto duration = std::chrono::milliseconds(1000);
  const auto stepTime = std::chrono::milliseconds(1);

  test(start, stop, step, aggTime, startTime, duration, stepTime);
}

TEST(SignalMediatorTest, test2) {
  const Frequency start = 144000000;
  const Frequency stop = 146000000;
  const Frequency step = 1000;
  const auto aggTime = std::chrono::seconds(1);
  const auto startTime = std::chrono::milliseconds(123456789);
  const auto duration = std::chrono::seconds(10);
  const auto stepTime = std::chrono::milliseconds(1);

  test(start, stop, step, aggTime, startTime, duration, stepTime);
}

TEST(SignalMediatorTest, test3) {
  const Frequency start = 144000000;
  const Frequency stop = 146000000;
  const Frequency step = 1000;
  const auto aggTime = std::chrono::seconds(1);
  const auto startTime = std::chrono::milliseconds(123456789);
  const auto duration = std::chrono::seconds(100);
  const auto stepTime = std::chrono::milliseconds(100);

  test(start, stop, step, aggTime, startTime, duration, stepTime);
}
