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
  BPlusTreeIndex(const string &index_file_path, const Compare &compare);
  ~BPlusTreeIndex();

 public:
  /**
   * @brief 插入值
   *
   * @param key 键
   * @param val 值
   * @return true
   * @return false
   */
  bool Insert(string_view key, string_view val);

  /**
   * @brief 删除值
   *
   * @param key 键
   * @return optional<address_t>
   */
  optional<address_t> Erase(string_view key);

  /**
   * @brief 搜索值
   *
   * @param key 键
   * @return optional<address_t>
   */
  optional<address_t> Search(string_view key);

 private:
  /**
   * @brief 定位键所在的叶子节点
   *
   * @param key
   * @return address_t
   */
  address_t LocateLeafNode(string_view key);

  /**
   * @brief 向叶子节点中插入新值
   *
   * @param page_address 地址
   * @param key 键
   * @param val 值
   * @return optional<address_t>
   */
  optional<address_t> InsertIntoLeafNode(address_t page_address,
                                         string_view key, string_view val);

  /**
   * @brief 向内部节点中插入新值
   *
   * @param page_address 地址
   * @param key 键
   * @param left_child 左孩子节点地址
   * @param right_child 有孩子节点地址
   * @return optional<address_t>
   */
  optional<address_t> InsertIntoInternalNode(address_t page_address,
                                             string_view key,
                                             address_t left_child,
                                             address_t right_child);

  /**
   * @brief 从叶子节点删除值
   *
   * @param page_address 地址
   * @param key 键
   */
  void EraseFromLeafNode(address_t page_address, string_view key);

  /**
   * @brief 从内部节点中删除值
   *
   * @param page_address
   * @param key
   */
  void EraseFromInteralNode(address_t page_address, string_view key);

 private:
  File *file_;
  BPlusTreeIndexMeta *index_meta_;
  Compare comparator_;
};
