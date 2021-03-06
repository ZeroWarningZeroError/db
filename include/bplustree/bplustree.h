#pragma once

#include <fmt/format.h>

#include <iostream>
#include <optional>
#include <string_view>

#include "basetype.h"
#include "bplustree/bplustree_page.h"
#include "code.h"
#include "file.h"

using std::optional;
using std::ostream;
using std::string_view;

class File;
class BufferedObjectMaker;
class IBufferPool;
class ISpaceManager;

struct BPlusTreeIndexMeta {
  address_t root;
  address_t leaf;
  address_t free;
  page_id_t max_page_id;
  uint32_t crc;
};

ostream &operator<<(ostream &os, const BPlusTreeIndexMeta &meta);

class BPlusTreeIndex {
 public:
  BPlusTreeIndex(const string &index_file_path, const Compare &compare,
                 IBufferPool *pool);
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
  ResultCode Insert(string_view key, string_view val);

  /**
   * @brief 删除值
   *
   * @param key 键
   * @return optional<address_t>
   */
  ResultCode Erase(string_view key);

  /**
   * @brief 搜索值
   *
   * @param key 键
   * @return optional<string>
   */
  optional<string> Search(string_view key);

 private:
  /**
   * @brief 定位键所在的叶子节点
   *
   * @param key
   * @return address_t
   */
  address_t LocateLeafNode(string_view key);

  ResultCode Insert(PageType page_type, address_t page_address, string_view key,
                    const vector<string_view> &vals);

  ResultCode Erase(PageType page_type, address_t page_address, string_view key);

  ResultCode EraseParentAndMergeSibling(PageType page_type,
                                        address_t left_child,
                                        address_t right_child);

  /**
   * @brief 修改页面孩子节点的父节点地址
   *
   * @param page
   */
  void ModifyParentAddressOfChildren(Page &page);

/**
 * @brief 记录页地址
 *
 */
#define BPLUSTREE_INDEX_PAGE_ADDRESS(page_id) \
  (sizeof(BPlusTreeIndexMeta) + PAGE_SIZE * page_id)

 public:
  void ScanLeafPage();
  void BFS();

 private:
  File *file_;
  BPlusTreeIndexMeta *index_meta_;
  Compare comparator_;
  string index_file_path_;
  BufferedObjectMaker *maker_;
  ISpaceManager *space_manager_;
  IBufferPool *pool_;
};