#pragma once

#include <iostream>
#include <string_view>

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

struct Record {
  string_view key;
  string_view val;
};

ostream &operator<<(ostream &os, const Record &record);