// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include <gtest/gtest.h>
#include <hwinfo/monitoring.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace std::chrono_literals;

TEST(Monitor, CallsCallbackPeriodicallyUntilStopped) {
  std::atomic<int> fetched{0};
  std::atomic<int> received{0};
  hwinfo::Monitor monitor{10ms, [&] { return ++fetched; }, [&](const int& value) { received = value; }};
  static_assert(std::is_same_v<decltype(monitor), hwinfo::Monitor<int>>);

  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (received < 3 && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(1ms);
  }
  EXPECT_TRUE(monitor.running());
  monitor.stop();
  EXPECT_FALSE(monitor.running());
  EXPECT_GE(received.load(), 3);

  const int after_stop = fetched;
  std::this_thread::sleep_for(50ms);
  EXPECT_EQ(fetched.load(), after_stop);
}

TEST(Monitor, StopDoesNotWaitForTheInterval) {
  const auto start = std::chrono::steady_clock::now();
  {
    hwinfo::Monitor monitor{1h, [] { return 0; }, [](const int&) {}};
  }  // destructor stops the monitor
  EXPECT_LT(std::chrono::steady_clock::now() - start, 10s);
}

TEST(Monitor, CanBeStoppedFromTheCallback) {
  std::atomic<int> calls{0};
  std::atomic<hwinfo::Monitor<int>*> self{nullptr};
  auto monitor = std::make_unique<hwinfo::Monitor<int>>(
      1ms, [] { return 0; },
      [&](const int&) {
        ++calls;
        while (self == nullptr) {  // the callback may run before make_unique returns
          std::this_thread::yield();
        }
        self.load()->stop();
      });
  self = monitor.get();
  std::this_thread::sleep_for(50ms);
  EXPECT_EQ(calls.load(), 1);
  monitor.reset();  // joins
}
