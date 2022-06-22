#pragma once
#include <gtest/gtest.h>

#include <functional>
#include <iostream>
#include <string>

#define PAGE_SIZE (16 * 1024)

#include "base.h"
#include "bplustree/page.h"
#include "memory/buffer.h"

using std::cout;
using std::endl;
using std::function;
using std::string;
using std::to_string;

class PageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    page_ = new Page(PageType::kLeafPage);
    for (int i = 0; i < node_size_; i++) {
      string key = "key" + to_string(i + start_record_no);
      string val = "val" + to_string(i);
      page_->Insert(key, {val}, cmp);
      insert_record_size_ += key.size() + val.size() + sizeof(RecordMeta);
    }
  }

  void TearDown() override { delete page_; }

  RecordData GetRecord(uint16_t record_address) {
    auto meta = page_->get_arribute<RecordMeta>(record_address);
    return {.key = {page_->base_address() + record_address + sizeof(RecordMeta),
                    meta->key_len},
            .val = {page_->base_address() + record_address +
                        sizeof(RecordMeta) + meta->key_len,
                    meta->val_len}};
  }

  Page *page_;

  int node_size_ = 100;
  int insert_record_size_ = 0;
  int virtual_record_size_ = sizeof(RecordMeta) * 2 + 3 * 2 + 8;
  int start_record_no = 10000;

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
  string search_key1 = "key" + to_string(start_record_no + node_size_ / 2);
  string expected_key1 =
      "key" + to_string(start_record_no + node_size_ / 2 - 1);

  uint16_t floor_record_address1 = page_->FloorSearch(search_key1, cmp);
  auto floor_record1 = GetRecord(floor_record_address1);
  ASSERT_EQ(expected_key1, floor_record1.key);

  string_view search_key3 = "key";
  string_view expected_key3 = "min";
  uint16_t floor_record_address3 = page_->FloorSearch(search_key3, cmp);
  auto floor_record3 = GetRecord(floor_record_address3);
  ASSERT_EQ(expected_key3, floor_record3.key);
}

TEST_F(PageTest, testInsertAndErase) {
  for (int i = start_record_no + node_size_;
       ResultCode::SUCCESS ==
       page_->Insert("key" + to_string(i), {"val" + to_string(i)}, cmp);
       i++)
    ;
  int first_insert_node_size = page_->meta()->node_size;
  for (int i = start_record_no + node_size_;
       ResultCode::SUCCESS == page_->Erase("key" + to_string(i), cmp); i++)
    ;

  ASSERT_EQ(node_size_, page_->meta()->node_size);

  for (int i = start_record_no + node_size_;
       ResultCode::SUCCESS ==
       page_->Insert("key" + to_string(i), {"val" + to_string(i)}, cmp);
       i++)
    ;
  int second_insert_node_size = page_->meta()->node_size;
  for (int i = start_record_no + node_size_;
       ResultCode::SUCCESS == page_->Erase("key" + to_string(i), cmp); i++)
    ;

  ASSERT_EQ(node_size_, page_->meta()->node_size);
}

TEST_F(PageTest, testSplitLeafPage) {
  auto [mid_key, other_page] = page_->SplitPage();

  int half = node_size_ / 2 + 1;
  int other_size = node_size_ - half;
  ASSERT_EQ(half, page_->meta()->node_size);
  ASSERT_EQ(other_size, other_page.meta()->node_size);
  ASSERT_EQ("key" + to_string(10000 + half - 1), mid_key);
}

TEST_F(PageTest, testSplitInternalPage) {
  page_->meta()->page_type = PageType::kInternalPage;
  auto [mid_key, other_page] = page_->SplitPage();

  int half = node_size_ / 2;
  int other_size = node_size_ - half;
  ASSERT_EQ(half, page_->meta()->node_size);
  ASSERT_EQ(other_size - 1, other_page.meta()->node_size);
  ASSERT_EQ("key" + to_string(10000 + half), mid_key);
}

TEST_F(PageTest, testLowerBound) {
  auto key1 = "key" + to_string(start_record_no + 10);
  auto expected_key1 = "key10010";
  auto record1 = GetRecord(page_->LowerBound(key1, cmp));
  ASSERT_EQ(expected_key1, record1.key);

  auto key2 = "key00000";
  auto expected_key2 = "key10000";
  auto record2 = GetRecord(page_->LowerBound(key2, cmp));
  ASSERT_EQ(expected_key2, record2.key);

  page_->Insert("key00001", {"val00001"}, cmp);
  page_->Insert("key00010", {"val00010"}, cmp);
  auto key3 = "key00005";
  auto expected_key3 = "key00010";
  auto record3 = GetRecord(page_->LowerBound(key3, cmp));
  ASSERT_EQ(expected_key3, record3.key);
}

TEST_F(PageTest, testBeginIterator) {
  auto iter = page_->Iterator();
  ASSERT_EQ(true, iter.isVirtualRecord());
  ASSERT_EQ(true, iter.hasNext());
  ASSERT_EQ("min", iter->key);
}

TEST_F(PageTest, testIteratorScan) {
  auto iter = page_->Iterator().Next();
  int no = 0;
  while (iter.isVirtualRecord()) {
    string expected_key = "key" + to_string(start_record_no + no);
    ASSERT_EQ(expected_key, iter->key);
    iter = iter.Next();
    no++;
  }
}

TEST_F(PageTest, testLocateLastRecord) {
  uint16_t record_address = page_->LocateLastRecord();
  RecordData data = GetRecord(record_address);
  ASSERT_EQ("val99", data.val);
}

TEST_F(PageTest, testAppendPage) {
  Page page(kLeafPage);
  int end_no = 110;
  for (int i = 100; i < end_no; i++) {
    string key = "key" + to_string(i + start_record_no);
    string val = "val" + to_string(i);
    page.Insert(key, {val}, cmp);
  }
  page_->AppendPage(page, cmp);
  uint16_t record_address = page_->LocateLastRecord();
  RecordData data = GetRecord(record_address);
  string expected_val = "val" + to_string(end_no - 1);
  ASSERT_EQ(expected_val, data.val);
}