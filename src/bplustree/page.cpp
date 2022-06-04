#include "bplustree/page.h"

#include <fmt/format.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "basetype.h"
#include "bplustree/record.h"
#include "serialize.h"

using std::cout;
using std::endl;
using std::make_shared;
using std::string;
using std::string_view;
using std::to_string;
#pragma region "PageMeta"

ostream &operator<<(ostream &os, const PageMeta &meta) {
  return os << fmt::format(
             "[type=Page, fields=[prev={}, next={}, heap_top={}[{}], free={}, "
             "use={}, slots={}, node_size={}, free_size={}]",
             meta.prev, meta.next, meta.heap_top, sizeof(PageMeta), meta.free,
             meta.use, meta.slots, meta.node_size, meta.free_size);
}

#pragma endregion

#pragma region "Page"

Page::Page(PageType page_type) {
  // 这种方式存疑
  char *raw_buffer = new char[PAGE_SIZE];
  memset(raw_buffer, 0, PAGE_SIZE);
  page_data_.reset(raw_buffer);

  meta_ = reinterpret_cast<PageMeta *>(page_data_.get());
  // 设置基本属性
  meta_->slots = 2;
  meta_->size = PAGE_SIZE;
  meta_->use = sizeof(PageMeta);
  meta_->page_type = page_type;

  base_address_ = page_data_.get();
  // 设置虚拟记录
  string virtual_min_key = "min";
  string virtual_max_key = "max";

  virtual_min_record_address_ = sizeof(PageMeta);
  vritual_max_record_address_ =
      sizeof(PageMeta) + sizeof(RecordMeta) + virtual_min_key.size();

  RecordMeta min_record_meta = {.next = vritual_max_record_address_,
                                .owned = 1,
                                .delete_mask = 0,
                                .len = 3,
                                .key_len = 3,
                                .val_len = 0,
                                .slot_no = 0};

  Record min_record = {{virtual_min_key}, {""}};
  RecordMeta max_record_meta = {.next = 0,
                                .owned = 1,
                                .delete_mask = 0,
                                .len = 11,
                                .key_len = 3,
                                .val_len = 8,
                                .slot_no = 0};

  Record max_record = {{virtual_max_key}, Serializer<address_t>::serialize(0)};

  uint16_t offset =
      this->set_record(sizeof(PageMeta), &min_record_meta, &min_record);
  uint16_t min_record_address = sizeof(PageMeta);
  this->Fill(this->slot_offset(0),
             Serializer<uint16_t>::serialize(min_record_address));

  uint16_t max_record_address = sizeof(PageMeta) + offset;
  offset += this->set_record(sizeof(PageMeta) + offset, &max_record_meta,
                             &max_record);
  this->Fill(this->slot_offset(1),
             Serializer<uint16_t>::serialize(max_record_address));

  meta_->heap_top = sizeof(PageMeta) + offset;
  meta_->free_size =
      meta_->size - meta_->heap_top - meta_->slots * sizeof(uint16_t);
}

/**
 * @brief 插入记录
 *
 * @param key 键
 * @param vals 值, 对应叶子节点vals包含一个值, 对于内部节点vals传两个值
 * @param compare 比较函数
 * @return bool 是否插入成功
 */
PageCode Page::Insert(string_view key, const vector<string_view> &vals,
                      const Compare &compare) {
  // 检查插入的值, 是否符合当前页面的要求
  int expected_val_size = meta_->page_type == kLeafPage ? 1 : 2;
  assert(expected_val_size == vals.size());

  // 提前尝试分配内存, 防止后续内存分配, 导致页面内存布局变化
  uint16_t record_occupy_size =
      key.size() + vals[0].size() + sizeof(RecordMeta);
  this->TryAlloc(record_occupy_size + 4);

  // 定位 < key 的最大记录
  uint16_t prev_record_address = this->FloorSearch(key, compare);
  auto prev_record_meta = this->get_arribute<RecordMeta>(prev_record_address);

  // 判断插入key是否存在
  uint16_t record_address = prev_record_meta->next;
  auto record_meta = this->get_arribute<RecordMeta>(prev_record_meta->next);

  string_view record_key = {base_address_ + record_address + sizeof(RecordMeta),
                            record_meta->key_len};

  if (compare(record_key, key) != 0) {
    // 如果不存在, 插入一条新记录
    int slot_no = prev_record_meta->owned == 0
                      ? prev_record_meta->slot_no
                      : (prev_record_meta->slot_no + 1);
    record_address = this->alloc(record_occupy_size);
    if (record_address == 0) {
      return PageCode::PAGE_FULL;
    }
    this->InsertRecord(prev_record_address, record_address, key, vals[0],
                       compare);
    record_meta = this->get_arribute<RecordMeta>(record_address);

    auto slot_record_meta = this->get_arribute<RecordMeta>(this->slot(slot_no));

    slot_record_meta->owned += 1;
    if (slot_record_meta->owned >= 8) {
      this->SplitSlot(slot_no);
    }
  } else {
    // 插入key已经存在的情况
    // 1. 对于叶子页, 不允许重复key, 返回插入失败
    if (meta_->page_type == PageType::kLeafPage) {
      return PageCode::PAGE_KEY_EXIST;
    }
    // 2. 对于内部页, 则修改当前key, 对应的值
    this->Fill(record_address + sizeof(RecordMeta) + key.size(), vals[0]);
  }

  auto next_record_meta = this->get_arribute<RecordMeta>(record_meta->next);
  // 内部节点的插入, 则需要考虑插入值额外逻辑
  if (meta_->page_type == PageType::kInternalPage) {
    cout << record_meta->next << endl;
    cout << *next_record_meta << endl;
    // cout << stoi(vals[1]) << endl;
    this->Fill(record_meta->next + sizeof(RecordMeta) +
                   next_record_meta->key_len,
               vals[1]);
  }
  return PageCode::PAGE_OK;
}

/**
 * @brief 删除记录
 *
 * @param key 记录对应的键
 * @param compare 比较函数
 * @return bool 是否删除成功
 */
bool Page::Erase(string_view key, const Compare &compare) {
  int hit_slot = this->LocateSlot(key, compare);
  uint16_t pre_address = this->slot(hit_slot - 1);
  auto prev = this->get_arribute<RecordMeta>(pre_address);
  RecordMeta *cur = nullptr;
  string_view data;
  while (prev) {
    cur = get_arribute<RecordMeta>(prev->next);
    data = {base_address_ + prev->next + sizeof(RecordMeta),
            static_cast<size_t>(cur->key_len)};
    if (compare(key, data) == 0) {
      break;
    }
    if (cur->owned) {
      break;
    }
    prev = cur;
  }

  // 未命中
  if (compare(key, data) != 0) {
    cout << *cur << endl;
    cout << "key=" << key << " 未命中!" << endl;
    return false;
  }

  uint16_t next_use_record = cur->next;
  uint16_t need_delete_record = prev->next;
  // 删除数据移出数据链表
  prev->next = next_use_record;
  auto own_header = this->get_arribute<RecordMeta>(this->slot(hit_slot));

  // 调整目录
  if (cur->owned && prev->owned) {
    this->EraseSlot(hit_slot);
  } else {
    if (cur->owned) {
      own_header = prev;
      own_header->owned = cur->owned;
      set_slot(hit_slot, reinterpret_cast<char *>(prev) - base_address_);
    }
    own_header->owned--;
  }

  // 删除数据移入空闲链表中
  auto free_header = this->get_arribute<RecordMeta>(offsetof(PageMeta, free));
  cur->owned = 0;
  cur->delete_mask = 1;
  cur->next = free_header->next;
  free_header->next = need_delete_record;

  meta_->free_size = meta_->free_size + sizeof(RecordMeta) + cur->len;
  meta_->node_size--;
  return true;
}

/**
 * @brief 搜索记录
 *
 * @param key 记录对应的键
 * @param compare 比较函数
 * @return optional<string_view>
 */
optional<string_view> Page::Search(string_view key, const Compare &compare) {
  uint16_t hit_slot = this->LocateSlot(key, compare);
  uint16_t pre_address = this->slot(hit_slot - 1);
  auto prev = this->get_arribute<RecordMeta>(pre_address);
  RecordMeta *cur = nullptr;
  string_view data;
  while (prev) {
    cur = get_arribute<RecordMeta>(prev->next);
    data = {base_address_ + prev->next + sizeof(RecordMeta),
            static_cast<size_t>(cur->key_len)};
    if (compare(key, data) == 0) {
      return {{base_address_ + prev->next + sizeof(RecordMeta) + cur->key_len,
               static_cast<size_t>(cur->val_len)}};
    }
    if (cur->owned) {
      break;
    }
    prev = cur;
  }
  return std::nullopt;
}

/**
 * @brief 分裂页
 *
 * @return pair<string_view, SharedBuffer<Page>>
 */
pair<string, Page> Page::SplitPage() {

  uint16_t half = (meta_->node_size) / 2;
  uint16_t prev_record_address = this->LocateRecord(half - 1);
  auto prev_record_meta = this->get_arribute<RecordMeta>(prev_record_address);

  auto mid_record_meta = this->get_arribute<RecordMeta>(prev_record_meta->next);

  auto mid_key = this->View(prev_record_meta->next + sizeof(RecordMeta),
                            mid_record_meta->key_len);

  auto mid_val = this->View(prev_record_meta->next + sizeof(RecordMeta) +
                                mid_record_meta->key_len,
                            mid_record_meta->val_len);

  uint16_t new_page_begin_record_address = prev_record_meta->next;

  uint16_t record_address = meta_->page_type == kInternalPage
                                ? mid_record_meta->next
                                : prev_record_meta->next;
  uint16_t end_address =
      meta_->page_type == kInternalPage ? 0 : this->slot(meta_->slots - 1);

  // 拷贝出新页
  Page page = this->CopyRecordToNewPage(record_address, end_address);

  // 对原始页面进行修改
  uint16_t rest_slots = prev_record_meta->slot_no + 1;
  auto cur_record_meta =
      this->get_arribute<RecordMeta>(this->slot(prev_record_meta->slot_no - 1));
  int owned = 0;
  while (cur_record_meta->next != new_page_begin_record_address) {
    owned++;
    cur_record_meta = this->get_arribute<RecordMeta>(cur_record_meta->next);
  }
  prev_record_meta->next = end_address;

  this->get_arribute<RecordMeta>(this->slot(meta_->slots - 1))->owned =
      owned + 1;
  size_t slot_size = sizeof(uint16_t) * (rest_slots - 1);
  uint16_t *page_slots = new uint16_t[meta_->slots];
  memcpy(page_slots, base_address_ + this->slot_offset(0), slot_size);
  meta_->slots = rest_slots;
  memcpy(base_address_ + this->slot_offset(0), page_slots, slot_size);

  if (meta_->page_type == kInternalPage) {
    this->Fill(this->slot(meta_->slots - 1) + sizeof(RecordMeta) + 3, mid_val);
  }

  delete[] page_slots;
  return {mid_key.data(), page};
}

bool Page::full() const { return true; }

/**
 * @brief 在目录页中, 定位数据对应槽
 *
 * @param key       数据
 * @param compare   对比函数
 * @return uint16_t 返回数据对应槽所有指向的地址
 */
uint16_t Page::LocateSlot(string_view key, const Compare &compare) noexcept {
  int lo = 0, hi = meta_->slots;
  while (lo < hi) {
    int mid = (lo + hi) >> 1;
    uint16_t slotAddress = this->slot(mid);
    auto header = this->get_arribute<RecordMeta>(this->slot(mid));

    string_view data = {base_address_ + slotAddress + sizeof(RecordMeta),
                        static_cast<size_t>(header->key_len)};

    // mid = 0, 虚拟记录-最小记录
    // mid = meta_->slots - 1, 虚拟记录-最大记录
    // 0 < mid < meta_->slots - 1, 普通用户记录

    int cmp_res = compare(data, key);

    if (mid == 0 || mid == meta_->slots - 1) {
      cmp_res = (mid ? -1 : 1);
    }

    if (cmp_res > 0) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return lo;
}

/**
 * @brief 分裂对应槽, 平分槽内记录
 *
 * @param slot_no 槽索引编号
 */
void Page::SplitSlot(int slot_no) noexcept {
  auto meta = this->get_arribute<RecordMeta>(this->slot(slot_no));
  if (meta->owned <= 1) {
    return;
  }
  // 拆分目录页
  int half = meta->owned / 2;
  meta->owned = meta->owned - half;
  uint16_t new_slot_address =
      *get_arribute<uint16_t>(this->slot(slot_no, half - 2));
  auto new_own_header = get_arribute<RecordMeta>(new_slot_address);
  new_own_header->owned = half;
  insert_slot(slot_no, new_slot_address);
}

/**
 * @brief 尝试获取空间
 *
 * @param size
 * @return true
 * @return false
 */
bool Page::TryAlloc(uint16_t size) noexcept {
  if (this->slot_offset(0) - meta_->heap_top < size) {
    if (meta_->free_size >= size) {
      this->TidyPage();
    }
  }

  if (this->slot_offset(0) - meta_->heap_top >= size) {
    return true;
  }

  return false;
}

/**
 * @brief 在页面内开辟可用空间
 *
 * @param size 开辟空间大小
 * @return uint16_t 返回相对于基地址的偏移量
 *
 * todo 增加 free_size 这部分逻辑可优化
 */
uint16_t Page::alloc(uint16_t size) noexcept {
  uint16_t free_address = 0;
  // “堆空间”有可用内存, 则在堆空间中开辟新空间
  //  size + 4 预留两个“目录”位置
  if (this->slot_offset(0) - meta_->heap_top < size + 4) {
    if (meta_->free_size >= size + 4) {
      this->TidyPage();
    }
  }

  if (this->slot_offset(0) - meta_->heap_top >= size + 4) {
    free_address = meta_->heap_top;
    meta_->heap_top += size;
    // cout << free_address << " " << meta_->heap_top << " " <<
    // this->slot_offset(0) << endl;
    return free_address;
  }

  return 0;

  // 方案二
  // uint16_t free = meta_->free;
  // uint16_t total_free_size = 0;
  // // 遍历空闲链表, 找到可以复用空闲空间
  // auto pre = get_arribute<RecordMeta>(offsetof(PageMeta, free));

  // while (pre && pre->next) {
  //   uint16_t cur_free_address = pre->next;
  //   auto cur = get_arribute<RecordMeta>(cur_free_address);
  //   if (sizeof(RecordMeta) + cur->len >= size) {
  //     cur->delete_mask = 0;
  //     pre->next = cur->next;
  //     cout << "reuse:" << cur_free_address << endl;
  //     return cur_free_address;
  //   }
  //   total_free_size += cur->len;
  //   pre = cur;
  // }

  // // 若无可以复用的空间, 但是总的空闲空间超过申请的空间,
  // // 则对整个“堆空间”进行清理
  // if (total_free_size > size) {
  //   this->TidyPage();
  //   free_address = meta_->heap_top;
  //   meta_->heap_top += size;
  // }

  return free_address;
}

/**
 * @brief 返回页面内存的基地址
 *
 * @return char* 页面内存的基地址
 */
char *Page::base_address() const noexcept { return base_address_; }

/**
 * @brief 获取槽内记录的值
 *
 * @param slot_no 槽索引编号
 * @return uint16_t
 */
uint16_t Page::SlotValue(int slot_no) noexcept {
  return *reinterpret_cast<uint16_t *>(meta_->size - (meta_->slots - slot_no) *
                                                         sizeof(uint16_t));
}

/**
 * @brief 插入槽
 *
 * @param slot_no 槽索引编号
 * @param value 值
 */
void Page::InsertSlot(int slot_no, uint16_t value) noexcept {}

/**
 * @brief 删除槽
 *
 * @param slot_no
 * @return uint16_t
 */
uint16_t Page::EraseSlot(int slot_no) noexcept {
  uint16_t base_slot = this->slot_offset(0);
  char *src = base_address_ + base_slot;
  char *dst = base_address_ + base_slot + sizeof(uint16_t);
  size_t move_size = sizeof(uint16_t) * slot_no;
  memmove(dst, src, move_size);
  meta_->slots--;
  meta_->free_size = meta_->free_size + sizeof(uint16_t);
}

/**
 * @brief 返回对应目录槽记录的地址
 *
 * @param pos 目录槽序号
 * @return uint16_t 目录槽记录的地址
 */
uint16_t Page::slot(int pos) const noexcept {
  assert(pos >= 0 && pos < meta_->slots);
  return *reinterpret_cast<uint16_t *>(base_address_ + meta_->size -
                                       meta_->slots * sizeof(uint16_t) +
                                       pos * sizeof(uint16_t));
}

/**
 * @brief 返回对应目录槽的地址
 *
 * @param pos 目录槽序号
 * @return uint16_t 相对基地址偏移量
 */
uint16_t Page::slot_offset(int pos) const noexcept {
  assert(pos >= 0 && pos < meta_->slots);
  return meta_->size - meta_->slots * sizeof(uint16_t) + pos * sizeof(uint16_t);
}

/**
 * @brief 查询目录槽的记录地址
 *
 * @param slot_no 目录槽编号
 * @param record_no 记录编号
 * @return uint16_t 记录地址
 */
uint16_t Page::slot(int slot_no, int record_no) {
  auto prev = get_arribute<RecordMeta>(this->slot(slot_no - 1));
  int no = 0;
  while (prev) {
    auto cur = get_arribute<RecordMeta>(prev->next);
    if (no == record_no) {
      return prev->next;
    }
    no++;
    prev = cur;
  }
  return 0;
}

/**
 * @brief 插入目录槽
 *
 * @param slot_no 目录槽编号
 * @param address 记录地址
 * @return uint16_t
 */
uint16_t Page::insert_slot(int slot_no, uint16_t address) noexcept {
  // cout << "insert_slot:" << slot_no << " " << address << endl;
  uint16_t src = this->slot_offset(0);
  uint16_t dst = src - sizeof(uint16_t);
  memcpy(base_address_ + dst, base_address_ + src, sizeof(uint16_t) * slot_no);
  memcpy(base_address_ + dst + slot_no * sizeof(uint16_t), &address,
         sizeof(uint16_t));
  meta_->slots++;
  meta_->free_size = meta_->free_size - sizeof(uint16_t);
  return 0;
}

/**
 * @brief 删除目录槽
 *
 * @param slot_no 目录槽编号
 * @return uint16_t
 */
uint16_t Page::erase_slot(int slot_no) noexcept {
  // meta_->size += sizeof(uint16_t);
  cout << "erase_slot" << slot_no << endl;
  uint16_t base_slot = this->slot_offset(0);
  uint16_t src = base_slot;
  uint16_t dst = base_slot + sizeof(uint16_t) * slot_no;
  memcpy(base_address_ + dst, base_address_ + src, sizeof(uint16_t) * slot_no);
  meta_->slots--;
  meta_->free_size = meta_->free_size + sizeof(uint16_t);
  return 0;
}

/**
 * @brief 修改目录槽记录地址
 *
 * @param slot_no 目录槽编号
 * @param address 记录地址
 */
void Page::set_slot(int slot_no, uint16_t address) noexcept {
  uint16_t slot_address = this->slot_offset(slot_no);
  memcpy(base_address_ + slot_address, &address, sizeof(uint16_t));
  return;
}

/**
 * @brief 设置记录
 *
 * @param offset 地址
 * @param meta  记录的元数据
 * @param record 记录内容
 * @return int 写入数据大小
 */
int Page::set_record(uint16_t offset, const RecordMeta *meta,
                     const Record *record) noexcept {
  memcpy(base_address_ + offset, meta, sizeof(RecordMeta));
  memcpy(base_address_ + offset + sizeof(RecordMeta), record->key.data(),
         meta->key_len);
  memcpy(base_address_ + offset + sizeof(RecordMeta) + meta->key_len,
         record->val.data(), meta->val_len);
  return sizeof(RecordMeta) + meta->len;
}

void Page::TidyPage() {
  // todo 优化
  char *buffer = new char[16 * 1024];
  uint16_t *slots = new uint16_t[meta_->slots - 2];
  uint16_t use = meta_->use;
  uint16_t offset = 0;
  uint16_t slot = 0;

  // 保留页面设置和虚拟记录
  uint16_t new_heap_top = sizeof(PageMeta) + sizeof(RecordMeta) * 2 + 2 * 3 + 8;
  auto prev = get_arribute<RecordMeta>(use);
  uint16_t next_record_address = prev->next;
  uint16_t pre_record_address = prev->next;

  int no = 0;
  while (next_record_address >= new_heap_top) {
    auto cur = get_arribute<RecordMeta>(next_record_address);
    uint16_t occupy = sizeof(RecordMeta) + cur->len;
    pre_record_address = next_record_address;
    next_record_address = cur->next;
    if (cur->next >= new_heap_top) {
      cur->next = new_heap_top + offset + occupy;
    }
    memcpy(buffer + offset, cur, occupy);
    string_view val = {buffer + offset + sizeof(RecordMeta) + cur->key_len,
                       cur->val_len};
    if (cur->owned) {
      slots[slot++] = new_heap_top + offset;
    }
    offset += occupy;
  }

  meta_->use = sizeof(PageMeta);
  meta_->free = 0;
  auto min_record = get_arribute<RecordMeta>(meta_->use);
  min_record->next = new_heap_top;
  meta_->heap_top = new_heap_top;

  memcpy(base_address_ + meta_->heap_top, buffer, offset);
  memcpy(base_address_ + this->slot_offset(1), slots,
         sizeof(uint16_t) * (meta_->slots - 2));

  meta_->heap_top += offset;
  delete[] buffer;
  delete[] slots;
}

/**
 * @brief 拷贝页内记录到新页
 *
 * @param begin_record_address
 * @param end_record_address
 * @return Page
 */
Page Page::CopyRecordToNewPage(uint16_t begin_record_address,
                               uint16_t end_record_address) {
  Page page(static_cast<PageType>(meta_->page_type));

  uint16_t *page_slots = new uint16_t[meta_->slots];
  page_slots[0] = sizeof(PageMeta);
  uint16_t new_page_slot_no = 1;
  uint16_t owned = 0;
  // 拷贝出新页
  uint16_t record_address = begin_record_address;

  auto min_record_meta = page.get_arribute<RecordMeta>(sizeof(PageMeta));
  min_record_meta->next = page.meta_->heap_top;

  while (record_address != end_record_address) {
    owned++;
    auto record_meta = this->get_arribute<RecordMeta>(record_address);
    Record record = {
        .key = {base_address_ + record_address + sizeof(RecordMeta),
                record_meta->key_len},
        .val = {base_address_ + record_address + sizeof(RecordMeta) +
                    record_meta->key_len,
                record_meta->val_len}};
    record_address = record_meta->next;

    uint16_t new_page_next_record_address =
        page.meta_->heap_top + sizeof(RecordMeta) + record_meta->len;

    if (record_meta->owned != 0) {
      record_meta->owned = owned;
      page_slots[new_page_slot_no++] = page.meta_->heap_top;
      owned = 0;
    }

    if (record_address != end_record_address) {
      record_meta->next = new_page_next_record_address;
    }
    page.set_record(page.meta_->heap_top, record_meta, &record);
    page.meta_->heap_top = new_page_next_record_address;
  }

  auto max_record_meta = page.get_arribute<RecordMeta>(page.slot(1));
  max_record_meta->owned = owned + 1;
  page_slots[new_page_slot_no++] = end_record_address;
  page.meta_->slots = new_page_slot_no;
  memcpy(page.base_address_ + page.slot_offset(0), page_slots,
         sizeof(uint16_t) * page.meta_->slots);
  return page;
}

/**
 * @brief 查找第一个大于等于指定key, 记录位置
 *
 * @param key 键值
 * @param compare 比较器
 * @return uint16_t 记录地址
 */
uint16_t Page::LowerBound(string_view key, const Compare &compare) noexcept {
  int hit_slot = this->LocateSlot(key, compare);
  uint16_t pre_address = this->slot(hit_slot - 1);
  auto prev = this->get_arribute<RecordMeta>(pre_address);
  RecordMeta *cur = nullptr;
  // 查询插入位置
  while (prev) {
    cur = this->get_arribute<RecordMeta>(prev->next);
    string_view data = {base_address_ + prev->next + sizeof(RecordMeta),
                        static_cast<size_t>(cur->key_len)};
    // 结束条件
    // 1.查询槽后最后一条记录
    // 2.找寻到第一条比插入键值大或等于的记录
    if (cur->owned || compare(key, data) >= 0) {
      break;
    }
    prev = cur;
  }
  cur->slot_no = hit_slot;
  return prev->next;
}

/**
 * @brief 查找第一个小于指定key最大记录的位置
 *
 * @param key 键值
 * @param compare 比较器
 * @return uint16_t 记录地址
 */
uint16_t Page::FloorSearch(string_view key, const Compare &compare) noexcept {
  int hit_slot = this->LocateSlot(key, compare);
  uint16_t pre_address = this->slot(hit_slot - 1);
  uint16_t prev_slot = hit_slot - 1;
  auto prev = this->get_arribute<RecordMeta>(pre_address);
  RecordMeta *cur = nullptr;

  // 查询插入位置
  while (prev) {
    cur = this->get_arribute<RecordMeta>(prev->next);
    string_view data = {base_address_ + prev->next + sizeof(RecordMeta),
                        static_cast<size_t>(cur->key_len)};
    // 结束条件
    // 1.查询槽后最后一条记录
    // 2.找寻到第一条比插入键值大或等于的记录
    if (cur->owned || compare(key, data) >= 0) {
      break;
    }

    prev_slot = hit_slot;
    pre_address = prev->next;
    prev = cur;
  }

  this->get_arribute<RecordMeta>(pre_address)->slot_no = prev_slot;
  return pre_address;
}

/**
 * @brief 定位记录地址
 *
 * @param record_no 记录编号
 * @return uint16_t 记录地址
 */
uint16_t Page::LocateRecord(uint16_t record_no) noexcept {
  // if (meta_->slots == 2) {
  //   return 0;
  // }

  uint16_t num = 0;
  uint16_t slot_no = 1;

  RecordMeta *record_meta = nullptr;
  RecordMeta *prev_record_meta = this->get_arribute<RecordMeta>(this->slot(0));

  while (slot_no < meta_->slots) {
    uint16_t slot_record_address = this->slot(slot_no++);
    record_meta = this->get_arribute<RecordMeta>(slot_record_address);
    if (num + record_meta->owned >= record_no) {
      record_meta = prev_record_meta;
      break;
    }
    num = num + record_meta->owned;
    prev_record_meta = record_meta;
  }

  uint16_t record_address = 0;

  while (num <= record_no && record_meta->next != 0) {
    num++;
    record_address = record_meta->next;
    record_meta = this->get_arribute<RecordMeta>(record_meta->next);
  }

  record_address == this->slot(meta_->slots - 1) ? 0 : record_address;

  if (record_address) {
    record_meta = this->get_arribute<RecordMeta>(record_address);
    record_meta->slot_no = slot_no - 1;
  }

  return record_address;
}

/**
 * @brief 插入数据
 *
 * @param prev_record_address 插入的位置
 * @param key
 * @param val
 * @param compare
 * @return uint16_t
 */
uint16_t Page::InsertRecord(uint16_t prev_record_address,
                            uint16_t record_address, string_view key,
                            string_view val, const Compare &compare) {
  auto prev = this->get_arribute<RecordMeta>(prev_record_address);

  Record new_record = {
      .key = key,
      .val = val,
  };

  uint16_t key_len = key.size();
  uint16_t val_len = val.size();
  uint16_t len = key.size() + val.size();

  RecordMeta meta = {.next = prev->next,
                     .owned = 0,
                     .delete_mask = 0,
                     .len = len,
                     .key_len = key_len,
                     .val_len = val_len,
                     .slot_no = 0};

  uint16_t new_record_size =
      sizeof(RecordMeta) + key.size() + new_record.val.size();

  prev->next = record_address;

  set_record(record_address, &meta, &new_record);

  this->meta_->free_size =
      this->meta_->free_size - sizeof(RecordMeta) - meta.len;
  this->meta_->node_size++;

  return record_address;
}

uint16_t Page::Fill(uint16_t offset, string_view data) {
  memcpy(base_address_ + offset, data.data(), data.length());
}

uint16_t Page::Fill(uint16_t offset, uint16_t len, IReader *reader) {
  reader->read(offset, base_address_, len);
}

string_view Page::View(uint16_t offset, size_t len) {
  return {base_address_ + offset, len};
}

/**
 * @brief 遍历页面中的节点
 *
 */
void Page::scan_use() noexcept {
  auto use = get_arribute<RecordMeta>(meta_->use);
  uint16_t offset = meta_->use;
  cout << "use=[" << endl;
  while (use) {
    string_view key = {base_address_ + sizeof(RecordMeta) + offset,
                       static_cast<size_t>(use->key_len)};
    string_view val = {base_address_ + sizeof(RecordMeta) + offset +
                           use->key_len,
                       static_cast<size_t>(use->val_len)};
    cout << " " << (*use) << fmt::format("[key={}, val={}]", key, val) << endl;
    offset = use->next;
    use = get_arribute<RecordMeta>(use->next);
  }
  cout << "]" << endl;
}

void Page::scan_free() noexcept {
  auto free = get_arribute<RecordMeta>(meta_->free);
  uint16_t offset = meta_->free;
  cout << "free=[" << endl;
  int no = 0;
  while (free) {
    string_view key = {base_address_ + sizeof(RecordMeta) + offset,
                       static_cast<size_t>(free->key_len)};
    string_view val = {base_address_ + sizeof(RecordMeta) + offset +
                           free->key_len,
                       static_cast<size_t>(free->val_len)};
    cout << " " << *free << fmt::format("[key={}, val={}]", key, val) << endl;
    offset = free->next;
    free = get_arribute<RecordMeta>(free->next);
  }
  cout << "]" << endl;
}

void Page::scan_slots() noexcept {
  cout << "slots=[" << endl;
  for (int i = 0; i < meta_->slots; i++) {
    auto header = get_arribute<RecordMeta>(this->slot(i));
    string_view key = {base_address_ + this->slot(i) + sizeof(RecordMeta),
                       static_cast<size_t>(header->key_len)};
    cout << " [i=" << i << ",offset=" << this->slot_offset(i)
         << ",header=" << this->slot(i) << "," << header << ", " << key << "]"
         << endl;
  }
  cout << "]" << endl;
}

#pragma endregion
