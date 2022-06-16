#pragma once

#include <iostream>
#include <string_view>

#include "iterator.h"

using std::ostream;
using std::string_view;

// 记录头
struct RecordMeta {
  uint16_t next;
  uint16_t owned;
  uint16_t delete_mask;
  uint16_t len;
  uint16_t key_len;
  uint16_t val_len;
  uint16_t slot_no;
};

ostream &operator<<(ostream &os, const RecordMeta &meta);

struct RecordData {
  string_view key;
  string_view val;
};

// todo 记得修改
struct Record : RecordMeta, RecordData {};

ostream &operator<<(ostream &os, const Record &record);

class Page;

template <>
class Iterator<Record> {
 public:
  Iterator() = default;
  Iterator(Page *page, uint16_t record_address);
  // todo 增加异常处理

 public:
  /**
   * @brief 是否有下一个元素
   *
   * @return true
   * @return false
   */
  virtual bool hasNext() { return record_.next; };

  /**
   * @brief 下一个元素
   *
   * @return Iterator
   */
  virtual Iterator Next() { return {page_, record_.next}; };

  virtual bool isEnd() { return address_ == 0; };

  /**
   * @brief 重载箭头运算符
   *
   * @return Record*
   */
  virtual Record *operator->() { return &record_; };

  virtual Record operator*() { return record_; }

  bool isVirtualRecord();

 private:
  Page *page_;
  Record record_;
  uint16_t address_;
};