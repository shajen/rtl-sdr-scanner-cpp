#include <core_manager.h>
#include <gtest/gtest.h>

TEST(CoreManagerTest, Empty) {
  constexpr auto CORES = 32;
  CoreManager cm(CORES);

  std::vector<std::unique_ptr<CoreManager::Core>> cores;
  for (int i = 0; i < CORES; ++i) {
    auto core = cm.getCore();
    EXPECT_NE(core, nullptr);
    cores.push_back(std::move(core));
  }

  for (int i = 0; i < CORES; ++i) {
    auto core = cm.getCore();
    EXPECT_EQ(core, nullptr);
  }

  cores.clear();
  for (int i = 0; i < CORES; ++i) {
    auto core = cm.getCore();
    EXPECT_NE(core, nullptr);
    cores.push_back(std::move(core));
  }

  for (int i = 0; i < CORES; ++i) {
    auto core = cm.getCore();
    EXPECT_EQ(core, nullptr);
  }
}
