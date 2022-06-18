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
 * @return ResultCode
 */
ResultCode BPlusTreeIndex::Erase(string_view key) {
  address_t leaf_node_address = this->LocateLeafNode(key);
  return this->Erase(PageType::kLeafPage, leaf_node_address, key);
}

/**
 * @brief 搜索值
 *
 * @param key 键
 * @return optional<address_t>
 */
optional<string> BPlusTreeIndex::Search(string_view key) {
  address_t leaf_node_address = this->LocateLeafNode(key);
  Page page(kLeafPage);
  file_->read(leaf_node_address, page.base_address(), PAGE_SIZE);
  return page.Search(key, comparator_);
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

  ResultCode code = page.Insert(key, vals, comparator_);

  // if (page_type == kInternalPage) {
  //   cout << "Internal" << endl;
  //   page.scan_use();
  // }

  if (ResultCode::ERROR_PAGE_FULL == code) {
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
      if (ResultCode::SUCCESS != page.Insert(key, vals, comparator_)) {
        return ResultCode::FAIL;
      }
    } else {
      if (ResultCode::SUCCESS != other_page.Insert(key, vals, comparator_)) {
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

ResultCode BPlusTreeIndex::Erase(PageType page_type, address_t page_address,
                                 string_view key) {
  Page page(page_type);
  file_->read(page_address, page.base_address(), PAGE_SIZE);

  // 检查是否有节点被删除
  if (ResultCode::ERROR_KEY_NOT_EXIST == page.Erase(key, comparator_)) {
    cout << "key_not_exist" << endl;
    return ResultCode::ERROR_KEY_NOT_EXIST;
  }

  // 如果有节点被删除, 需要考虑页节点过少情况, 页合并的问题

  auto meta = page.meta();
  // 如果根节点为空, 则将根节点往下移动
  if (index_meta_->root == meta->self && meta->node_size == 0) {
    // 更新根节点, 修改子节点的父节点地址
    string_view child_address = page.Value(page.virtual_max_record_address_);
    index_meta_->root = Serializer<address_t>::deserialize(
        {child_address.data(), child_address.length()});
    Page child(kLeafPage);
    file_->read(index_meta_->root, child.base_address(), PAGE_SIZE);
    child.meta()->parent = 0;
    file_->write(index_meta_->root, child.base_address(), PAGE_SIZE);
    return ResultCode::SUCCESS;
  }

  file_->write(meta->self, page.base_address(), PAGE_SIZE);

  // 页合并的下限
  uint16_t data_size =
      meta->size - Page::VIRTUAL_MIN_RECORD_SIZE - sizeof(PageMeta);

  cout << "free_size=" << meta->free_size << ",data_size=" << data_size
       << ",factor=" << (meta->free_size * 1.0 / data_size) << endl;
  if (meta->free_size * 1.0 / data_size >= 0.8) {
    Page sibling_page(page_type);
    Page parent(kInternalPage);

    ResultCode code = ResultCode::SUCCESS;

    if (meta->next != 0) {
      code = EraseParentAndMergeSibling(page_type, page_address, meta->next);
    }

    if (ResultCode::ERROR_NOT_MATCH_CONSTRAINT == code && meta->prev != 0) {
      code = EraseParentAndMergeSibling(page_type, meta->prev, page_address);
    }

    if (ResultCode::ERROR_NOT_MATCH_CONSTRAINT != code) {
      return code;
    }
  }
  return ResultCode::SUCCESS;
}

ResultCode BPlusTreeIndex::EraseParentAndMergeSibling(
    PageType page_type, address_t left_child_address,
    address_t right_child_address) {
  Page left_child(page_type), right_child(page_type), parent(kInternalPage);
  file_->read(left_child_address, left_child.base_address(), PAGE_SIZE);
  file_->read(right_child_address, right_child.base_address(), PAGE_SIZE);

  cout << "left_child_address=" << left_child_address << ",free_size"
       << left_child.meta()->free_size << endl;
  cout << "right_child_address=" << right_child_address << ",data_size"
       << right_child.ValidDataSize() << endl;
  if (left_child.meta()->free_size < right_child.ValidDataSize()) {
    return ResultCode::ERROR_NOT_MATCH_CONSTRAINT;
  }

  if (left_child.meta()->parent != right_child.meta()->parent) {
    return ResultCode::ERROR_NOT_MATCH_CONSTRAINT;
  }

  left_child.scan_use();

  // file_->read(left_child.meta()->parent, parent.base_address(), PAGE_SIZE);

  // auto first_key_iter = right_child.Iterator().Next();

  // auto parent_key_address =
  //     parent.FloorSearch(first_key_iter->key, comparator_);

  // auto parent_key = parent.Key(parent_key_address);

  // if (PageType::kInternalPage == page_type) {
  //   left_child.Append(parent_key, first_key_iter->val, comparator_);
  // }

  // left_child.AppendPage(right_child, comparator_);
  // left_child.meta()->self = right_child.meta()->self;

  // file_->write(left_child.meta()->self, left_child.base_address(),
  // PAGE_SIZE); return this->Erase(PageType::kInternalPage,
  // left_child.meta()->parent,
  //                    parent_key);
  return ResultCode::SUCCESS;
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
          cout << "(" << key;
          if (key != "min") {
            cout << ", " << val;
            q.push(val);
          } else {
            cout << ", none";
          }
          cout << ")";
        }

        offset = use->next;
        use = page.get_arribute<RecordMeta>(use->next);
      }
      cout << "]" << endl;
    }
    d++;
    // if (d == 3) {
    //   return;
    // }
  }
}