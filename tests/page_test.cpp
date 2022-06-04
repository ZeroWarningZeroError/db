#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

#include "base.h"
#include "bplustree/page.h"
#include "memory/buffer.h"

using std::function;
using std::string;
using std::to_string;

class PageTest : public ::testing::Test {
protected:
  void SetUp() override {
    page_ = new Page(PageType::kLeafPage);
    for (int i = 0; i < 100; i++) {
      string key = "key" + to_string(i);
      string val = "val" + to_string(i);
      page_->Insert(key, {val}, cmp);
      insert_record_size_ += key.size() + val.size() + sizeof(RecordMeta);
    }
  }

  void TearDown() override { delete page_; }

  Record GetRecord(uint16_t record_address) {
    auto meta = page_->get_arribute<RecordMeta>(record_address);
    return {.key = {page_->base_address() + record_address + sizeof(RecordMeta),
                    meta->key_len},
            .val = {page_->base_address() + record_address + meta->key_len,
                    meta->val_len}};
  }

  Page *page_;

  int node_size_ = 100;
  int insert_record_size_ = 0;
  int virtual_record_size_ = sizeof(RecordMeta) * 2 + 3 * 2 + 8;

  function<int(string_view, string_view)> cmp = [](string_view key1,
                                                   string_view key2) -> int {
    if (key1 == key2) {
      return 0;
    }
    return key1 < key2 ? 1 : -1;
  };
};

TEST_F(PageTest, testNodeSize) {
  ASSERT_EQ(node_size_, page_->meta()->node_size);
}

TEST_F(PageTest, testFreeSize) {
  int expected_free_size = PAGE_SIZE - sizeof(PageMeta) -
                           sizeof(uint16_t) * page_->meta()->slots -
                           insert_record_size_ - virtual_record_size_;
  ASSERT_EQ(expected_free_size, page_->meta()->free_size);
}

TEST_F(PageTest, testFloorSearch) {
  string_view search_key1 = "key99";
  string_view expected_key1 = "key98";

  uint16_t floor_record_address1 = page_->FloorSearch(search_key1, cmp);
  Record floor_record1 = GetRecord(floor_record_address1);
  ASSERT_EQ(expected_key1, floor_record1.key);

  string_view search_key2 = "key120";
  string_view expected_key2 = "key12";
  uint16_t floor_record_address2 = page_->FloorSearch(search_key2, cmp);
  Record floor_record2 = GetRecord(floor_record_address2);
  ASSERT_EQ(expected_key2, floor_record2.key);

  string_view search_key3 = "key";
  string_view expected_key3 = "min";
  uint16_t floor_record_address3 = page_->FloorSearch(search_key3, cmp);
  Record floor_record3 = GetRecord(floor_record_address3);
  ASSERT_EQ(expected_key3, floor_record3.key);
}

TEST_F(PageTest, testInsertAndErase) {

  for (int i = 1000;
       PageCode::PAGE_OK ==
       page_->Insert("key" + to_string(i), {"val" + to_string(i)}, cmp);
       i++)
    ;
  int first_insert_node_size = page_->meta()->node_size;
  for (int i = 1000; page_->Erase("key" + to_string(i), cmp); i++)
    ;

  ASSERT_EQ(node_size_, page_->meta()->node_size);

  for (int i = 1000;
       PageCode::PAGE_OK ==
       page_->Insert("key" + to_string(i), {"val" + to_string(i)}, cmp);
       i++)
    ;
  int second_insert_node_size = page_->meta()->node_size;
  for (int i = 1000; page_->Erase("key" + to_string(i), cmp); i++)
    ;

  ASSERT_EQ(first_insert_node_size, second_insert_node_size);
  ASSERT_EQ(node_size_, page_->meta()->node_size);
}

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}