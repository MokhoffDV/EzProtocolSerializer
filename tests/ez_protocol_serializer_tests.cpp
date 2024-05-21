#include <gtest/gtest.h>

TEST(InitialTestSuite, ThisShouldFail) {
  EXPECT_EQ(1, 1);
  EXPECT_EQ(1, 0);
}