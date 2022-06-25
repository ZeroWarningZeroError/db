#include <gtest/gtest.h>

#include "buffer/replacer.h"

class LRUReplacerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    capacity_ = 10;
    replacer_ = new LRUReplacer(10);

    for (int i = 0; i < 5; i++) {
      replacer_->Unpin(i);
    }
  }
  void TearDown() override {}

 protected:
  IReplacer *replacer_;
  int capacity_;
};

TEST_F(LRUReplacerTest, testEvict) {
  optional<frame_id_t> result = replacer_->Victim();
  ASSERT_EQ(true, result.has_value());
  ASSERT_EQ(4, result.value());
}
