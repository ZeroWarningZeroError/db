#include "bplustree.h"

#include <cassert>
#include <iostream>
#include <queue>

#include "bplustree/page.h"
#include "serialize.h"

using std::cout;
using std::endl;
using std::queue;

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
  index_meta_ = new BPlusTreeIndexMeta{0, 0, 0, 0, 0};
  if (is_init) {
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
ResultCode BPlusTreeIndex::Insert(string_view key, string_view val) {
  address_t leaf_node_address = this->LocateLeafNode(key);
  return this->Insert(PageType::kLeafPage, leaf_node_address, key, {val});
}

/**
 * @brief 删除值
 *
 * @param key 键
 * @return optional<address_t>
 */
ResultCode BPlusTreeIndex::Erase(string_view key) { return ResultCode::OK; }

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
  if (index_meta_->root == 0) {
    return 0;
  }

  Page page(PageType::kLeafPage);
  file_->read(index_meta_->root, page.base_address(), PAGE_SIZE);

  address_t target_node_address = index_meta_->root;
  while (page.meta()->page_type == PageType::kInternalPage) {
    // cout << target_node_address << endl;
    uint16_t offset = page.LowerBound(key, comparator_);
    auto meta = page.get_arribute<RecordMeta>(offset);
    // auto key = {page.base_address() + sizeof(RecordMeta), meta->key_len};
    target_node_address = *(page.get_arribute<address_t>(
        offset + sizeof(RecordMeta) + meta->key_len));
    // cout <<
    file_->read(target_node_address, page.base_address(), PAGE_SIZE);
  }

  return target_node_address;
}

ResultCode BPlusTreeIndex::Insert(PageType page_type, address_t page_address,
                                  string_view key,
                                  const vector<string_view> &vals) {
  Page page = Page(page_type);
  if (page_address != 0) {
    file_->read(page_address, page.base_address(), PAGE_SIZE);
  } else {
    index_meta_->root =
        BPLUSTREE_INDEX_PAGE_ADDRESS(index_meta_->max_page_id++);
    page.meta()->self = index_meta_->root;
    if (page_type == kLeafPage) {
      index_meta_->leaf = index_meta_->root;
    }
  }

  PageCode code = page.Insert(key, vals, comparator_);

  // if (page_type == kInternalPage) {
  //   cout << "Internal" << endl;
  //   page.scan_use();
  // }

  if (PageCode::PAGE_FULL == code) {
    // // page.scan_use();
    // cout << "SplitPage:" << key << endl;
    // if (key == "key1010") {
    //   cout << "debug" << endl;
    // }
    // page.scan_use();
    // page.scan_slots();
    auto [mid_key, other_page] = page.SplitPage();

    // cout << "Insert:mid_key=" << mid_key << ",key=" << key << endl;
    // other_page.scan_use();
    // other_page.scan_slots();

    if (key < mid_key) {
      if (PageCode::PAGE_OK != page.Insert(key, vals, comparator_)) {
        return ResultCode::FAIL;
      }
    } else {
      if (PageCode::PAGE_OK != other_page.Insert(key, vals, comparator_)) {
        return ResultCode::FAIL;
      }
    }
    // cout << "Insert Mid" << endl;

    other_page.meta()->self =
        BPLUSTREE_INDEX_PAGE_ADDRESS(index_meta_->max_page_id++);
    page.meta()->next = other_page.meta()->self;

    string left_child_address =
        Serializer<address_t>::serialize(page.meta()->self);
    string right_child_address =
        Serializer<address_t>::serialize(other_page.meta()->self);

    address_t parent_address = page.meta()->parent;
    if (page.meta()->parent == 0) {
      address_t alloc_parent_address =
          BPLUSTREE_INDEX_PAGE_ADDRESS(index_meta_->max_page_id);
      page.meta()->parent = alloc_parent_address;
      other_page.meta()->parent = alloc_parent_address;
    } else {
      other_page.meta()->parent = parent_address;
    }

    file_->write(page.meta()->self, page.base_address(), PAGE_SIZE);
    file_->write(other_page.meta()->self, other_page.base_address(), PAGE_SIZE);

    if (page_type == kInternalPage) {
      this->ModifyParentAddressOfChildren(other_page);
    }

    // page.scan_use();
    // page.scan_slots();
    // other_page.scan_use();
    // other_page.scan_slots();
    return this->Insert(PageType::kInternalPage, parent_address, mid_key,
                        {left_child_address, right_child_address});

    // return ResultCode::NODE_FULL;
  }

  file_->write(page.meta()->self, page.base_address(), PAGE_SIZE);
  return ResultCode::SUCCESS;
}

/**
 * @brief 从叶子节点删除值
 *
 * @param page_address 地址
 * @param key 键
 */
ResultCode BPlusTreeIndex::EraseFromLeafNode(address_t page_address,
                                             string_view key) {
  if (page_address == 0) {
    return ResultCode::OK;
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
        return this->EraseFromLeafNode(meta->parent, key);
      }
    }

    if (meta->prev != 0) {
      file_->read(meta->prev, sbling_page.base_address(), PAGE_SIZE);
      if (meta->free_size >
          sbling_page.meta()->size - sbling_page.meta()->free_size) {
        page.MergePage(sbling_page, true);
        return this->EraseFromLeafNode(meta->parent, key);
      }
    }
  }
  return ResultCode::OK;
}

/**
 * @brief 从内部节点中删除值
 *
 * @param page_address
 * @param key
 */
ResultCode BPlusTreeIndex::EraseFromInteralNode(address_t page_address,
                                                string_view key) {
  return ResultCode::OK;
}

ResultCode BPlusTreeIndex::Erase(PageType page_type, address_t page_address,
                                 string_view key) {
  return ResultCode::OK;
}

/**
 * @brief 修改页面孩子节点的父节点地址
 *
 * @param page
 */
void BPlusTreeIndex::ModifyParentAddressOfChildren(Page &page) {
  uint16_t record_address =
      page.get_arribute<RecordMeta>(page.meta()->use)->next;
  Page child_page(kLeafPage);
  cout << "ModifyParentAddressOfChildren:";
  while (record_address != 0) {
    auto record_meta = page.get_arribute<RecordMeta>(record_address);
    address_t child_address = *(page.get_arribute<address_t>(
        record_address + sizeof(RecordMeta) + record_meta->key_len));
    cout << "(" << child_address << "," << page.meta()->self << ")";
    file_->read(child_address, child_page.base_address(), PAGE_SIZE);
    child_page.meta()->parent = page.meta()->self;
    file_->write(child_address, child_page.base_address(), PAGE_SIZE);
    record_address = record_meta->next;
  }
  cout << endl;
  return;
}

void BPlusTreeIndex::ScanLeafPage() {
  address_t leaf_node_address = index_meta_->leaf;
  while (leaf_node_address) {
    Page page(kLeafPage);
    file_->read(leaf_node_address, page.base_address(), PAGE_SIZE);
    leaf_node_address = page.meta()->next;
    cout << "leaf_node_address=" << leaf_node_address << endl;
    page.scan_use();
  }
}

void BPlusTreeIndex::BFS() {
  queue<address_t> q;
  q.push(index_meta_->root);
  int d = 0;
  while (q.size()) {
    int size = q.size();
    for (int i = 0; i < size; i++) {
      for (int j = 0; j < d; j++) {
        cout << "\t";
      }
      // cout << q.front() << endl;
      Page page(kInternalPage);
      // cout << q.front() << " ";
      file_->read(q.front(), page.base_address(), PAGE_SIZE);
      q.pop();
      auto use = page.get_arribute<RecordMeta>(page.meta()->use);
      uint16_t offset = page.meta()->use;
      cout << "Page[meta=" << *page.meta();
      while (use) {
        string_view key = {page.base_address() + sizeof(RecordMeta) + offset,
                           static_cast<size_t>(use->key_len)};
        if (page.meta()->page_type == kLeafPage) {
          string_view val = {
              page.base_address() + sizeof(RecordMeta) + offset + use->key_len,
              static_cast<size_t>(use->val_len)};
          cout << "(" << key << ", " << val << ")";
        } else {
          address_t val = *reinterpret_cast<address_t *>(
              page.base_address() + sizeof(RecordMeta) + offset + use->key_len);
          cout << "(" << key << ", " << val << ")";
          if (key != "min") {
            q.push(val);
          }
        }

        offset = use->next;
        use = page.get_arribute<RecordMeta>(use->next);
      }
      cout << "]" << endl;
    }
    d++;
    if (d == 3) {
      return;
    }
  }
}