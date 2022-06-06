#include "bplustree.h"

#include <cassert>
#include <iostream>

#include "bplustree/page.h"
#include "serialize.h"

using std::cout;
using std::endl;

ostream &operator<<(ostream &os, const BPlusTreeIndexMeta &meta) {
  os << fmt::format("[root={},leaf={},free={},crc={}]", meta.root, meta.leaf,
                    meta.free, meta.crc);
  return os;
}

BPlusTreeIndex::BPlusTreeIndex(const string &index_file_path,
                               const Compare &compare)
    : comparator_(compare) {
  // 打开索引文件
  bool is_init = File::exist(index_file_path);
  file_ = new File(index_file_path);
  assert(file_->is_open());

  // 加载索引的元数据
  index_meta_ = new BPlusTreeIndexMeta{0, 0, 0, 0};
  if (!is_init) {
    file_->write(0, index_meta_, sizeof(BPlusTreeIndexMeta));
    index_meta_->root = this->AllocEmptyPage(PageType::kLeafPage);
    index_meta_->leaf = index_meta_->root;
  } else {
    file_->read(0, index_meta_, sizeof(BPlusTreeIndexMeta));
  }
  cout << (*index_meta_) << endl;
}

BPlusTreeIndex::~BPlusTreeIndex() {
  if (index_meta_ != nullptr) {
    cout << (*index_meta_) << endl;
    file_->write(0, index_meta_, sizeof(BPlusTreeIndexMeta));
    delete index_meta_;
    delete file_;
  }
}

/**
 * @brief 插入值
 *
 * @param key 键
 * @param val 值
 * @return true
 * @return false
 */
bool BPlusTreeIndex::Insert(string_view key, string_view val) {
  address_t leaf_node_address = this->LocateLeafNode(key);
  this->InsertIntoLeafNode(leaf_node_address, key, val);
  return true;
}

/**
 * @brief 删除值
 *
 * @param key 键
 * @return optional<address_t>
 */
optional<address_t> BPlusTreeIndex::Erase(string_view key) {
  return std::nullopt;
}

/**
 * @brief 搜索值
 *
 * @param key 键
 * @return optional<address_t>
 */
optional<address_t> BPlusTreeIndex::Search(string_view key) {
  return std::nullopt;
}

/**
 * @brief 根据键值定位叶子节点的地址
 *
 * @param key 键
 * @param comparator_ 比较器
 * @return address_t 叶子节点地址
 */
address_t BPlusTreeIndex::LocateLeafNode(string_view key) {
  Page page(PageType::kLeafPage);
  file_->read(index_meta_->root, page.base_address(), PAGE_SIZE);

  address_t target_node_address = index_meta_->root;
  while (page.meta()->page_type == PageType::kInternalPage) {
    uint16_t offset = page.LowerBound(key, comparator_);
    auto meta = page.get_arribute<RecordMeta>(offset);
    target_node_address = *(page.get_arribute<address_t>(
        offset + sizeof(RecordMeta) + meta->key_len));
    file_->read(target_node_address, page.base_address(), PAGE_SIZE);
  }

  return target_node_address;
}

/**
 * @brief 向叶子节点中插入新值
 *
 * @param page_address 地址
 * @param key 键
 * @param val 值
 * @return optional<address_t>
 */
optional<address_t> BPlusTreeIndex::InsertIntoLeafNode(address_t page_address,
                                                       string_view key,
                                                       string_view val) {
  Page page(PageType::kLeafPage);
  file_->read(page_address, page.base_address(), PAGE_SIZE);

  PageCode code = page.Insert(key, {val}, comparator_);

  if (PageCode::PAGE_OK == code) {
    page.scan_use();
    file_->write(page_address, page.base_address(), PAGE_SIZE);
    return std::nullopt;
  }

  if (PageCode::PAGE_FULL == code) {
    auto [mid_key, other_page] = page.SplitPage();
    return this->InsertIntoInternalNode(
        page.meta()->parent, key, page.meta()->self, other_page.meta()->self);
  }

  return std::nullopt;
}

/**
 * @brief 向内部节点中插入新值
 *
 * @param page_address 地址
 * @param key 键
 * @param left_child 左孩子节点地址
 * @param right_child 有孩子节点地址
 * @return optional<address_t>
 */
optional<address_t> BPlusTreeIndex::InsertIntoInternalNode(
    address_t page_address, string_view key, address_t left_child,
    address_t right_child) {
  Page page = Page(PageType::kInternalPage);
  if (page_address != 0) {
    file_->read(page_address, page.base_address(), PAGE_SIZE);
  } else {
    page.meta()->self = file_->alloc(PAGE_SIZE);
    index_meta_->root = page.meta()->self;
  }

  PageCode code = page.Insert(key,
                              {Serializer<address_t>::serialize(left_child),
                               Serializer<address_t>::serialize(right_child)},
                              comparator_);

  if (PageCode::PAGE_OK == code) {
    return std::nullopt;
  }

  if (PageCode::PAGE_FULL == code) {
    auto [mid_key, other_page] = page.SplitPage();
    return this->InsertIntoInternalNode(
        page.meta()->parent, key, page.meta()->self, other_page.meta()->self);
  }

  file_->write(page.meta()->self, page.base_address(), PAGE_SIZE);
}

/**
 * @brief 从叶子节点删除值
 *
 * @param page_address 地址
 * @param key 键
 */
void BPlusTreeIndex::EraseFromLeafNode(address_t page_address,
                                       string_view key) {
  if (page_address == 0) {
    return;
  }
  Page page = Page(PageType::kInternalPage);
  file_->read(page_address, page.base_address(), PAGE_SIZE);

  page.Erase(key, comparator_);

  auto meta = page.meta();

  if (meta->free_size * 1.0 / meta->size < 0.2) {
    Page sbling_page(PageType::kInternalPage);
    if (meta->next != 0) {
      file_->read(meta->next, sbling_page.base_address(), PAGE_SIZE);
      if (meta->free_size >
          sbling_page.meta()->size - sbling_page.meta()->free_size) {
        page.MergePage(sbling_page, true);
        this->EraseFromLeafNode(meta->parent, key);
        return;
      }
    }

    if (meta->prev != 0) {
      file_->read(meta->prev, sbling_page.base_address(), PAGE_SIZE);
      if (meta->free_size >
          sbling_page.meta()->size - sbling_page.meta()->free_size) {
        page.MergePage(sbling_page, true);
        this->EraseFromLeafNode(meta->parent, key);
        return;
      }
    }
  }
}

/**
 * @brief 从内部节点中删除值
 *
 * @param page_address
 * @param key
 */
void BPlusTreeIndex::EraseFromInteralNode(address_t page_address,
                                          string_view key) {
  return;
}

/**
 * @brief 开辟空闲页
 *
 * @param page_type
 * @return address_t
 */
address_t BPlusTreeIndex::AllocEmptyPage(PageType page_type) {
  address_t page_address = file_->alloc(PAGE_SIZE);
  Page page(page_type);
  page.meta()->self = page_address;
  file_->write(page_address, page.base_address(), PAGE_SIZE);
  return page_address;
};
