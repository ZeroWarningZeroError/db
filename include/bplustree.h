#pragma once

#include <fmt/format.h>

#include <iostream>
#include <optional>
#include <string_view>

#include "basetype.h"
#include "file.h"

using std::optional;
using std::ostream;
using std::string_view;

class File;

struct BPlusTreeIndexMeta {
  uint32_t root;
  uint32_t leaf;
  uint32_t free;
  uint32_t crc;
};

ostream &operator<<(ostream &os, const BPlusTreeIndexMeta &meta);

class BPlusTreeIndex {
public:
  BPlusTreeIndex(const string &index_file_path);
  ~BPlusTreeIndex();

public:
  bool insert(string_view key, string_view val, const Compare &compare);
  optional<address_t> erase(string_view key);

private:
  address_t locate_leaf_node(string_view key, const Compare &compare);

  optional<address_t> InsertLeafNode(address_t page_address, string_view key,
                                     string_view val, const Compare &compare);

  optional<address_t> InsertInternalNode(address_t page_address,
                                         string_view key, address_t left_child,
                                         address_t right_child,
                                         const Compare &compare);

  void EraseLeafNode(address_t page_address, string_view key,
                     const Compare &compare);

private:
  File *file_;
  BPlusTreeIndexMeta *index_meta_;
};
