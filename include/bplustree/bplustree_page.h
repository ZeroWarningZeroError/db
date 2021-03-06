#pragma once
#ifndef PAGE_H
#define PAGE_H

#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base.h"
#include "basetype.h"
#include "bplustree/record.h"
#include "buffer/buffer_pool.h"
#include "buffer/construct.h"
#include "buffer/extend_frame.h"
#include "code.h"
#include "memory/buffer.h"

using std::optional;
using std::ostream;
using std::pair;
using std::shared_ptr;
using std::string;
using std::string_view;
using std::vector;

/**
 * @brief 磁盘页面元数据
 */
struct PageMeta {
  // 页面地址
  uint32_t self;
  // 父节点地址
  uint32_t parent;
  // 前驱节点地址
  uint32_t prev;
  // 后继节点地址
  uint32_t next;
  // 页面堆堆顶偏移量
  uint16_t heap_top;
  // 空闲记录头节点
  uint16_t free;
  // 记录节点
  uint16_t use;
  uint16_t directory;
  // 目录槽的数量
  uint16_t slots;
  // 页面类型
  uint16_t page_type;
  // 节点数量
  uint16_t node_size;
  // 页在父节点对应的位置
  uint16_t parent_key_offset;
  // 可用空间
  uint32_t free_size;
  // 页面大小
  uint32_t size;
};

ostream &operator<<(ostream &os, const PageMeta &meta);

/**
 * @brief 页面类型
 *
 */
enum PageType { kLeafPage = 0, kInternalPage = 1 };

enum class PageCode {
  PAGE_FULL,
  PAGE_KEY_EXIST,
  PAGE_OK,
  PAGE_FAIL,
  PAGE_SUCCESS
};

/**
 * @brief 磁盘页面
 */
class Page {
 public:
  Page(PageType page_type, bool isLoad = false, char *buffer = nullptr);

 public:
  /**
   * @brief 插入记录
   *
   * @param key 键
   * @param vals 值, 对应叶子节点vals包含一个值, 对于内部节点vals传两个值
   * @param compare 比较函数
   * @return ResultCode 返回插入状态
   */
  ResultCode Insert(string_view key, const vector<string_view> &vals,
                    const Compare &compare);

  /**
   * @brief 删除记录
   *
   * @param key 记录对应的键
   * @param compare 比较函数
   * @return bool 是否删除成功
   */
  ResultCode Erase(string_view key, const Compare &compare);

  /**
   * @brief 添加记录
   *
   * @param key
   * @param val
   * @param compare
   * @return ResultCode
   */
  ResultCode Append(string_view key, string_view val, const Compare &compare);

  /**
   * @brief 添加记录
   *
   * @param page 页记录
   * @param compare
   * @return ResultCode
   */
  ResultCode AppendPage(Page &page, const Compare &compare);

  /**
   * @brief 搜索记录
   *
   * @param key 记录对应的键
   * @param compare 比较函数
   * @return optional<string>
   */
  optional<string> Search(string_view key,
                          const Compare &compare) const noexcept;

  /**
   * @brief 分裂页
   *
   * @return pair<string, Page>
   */
  pair<string, Page> SplitPage();

  /**
   * @brief 合并页
   *
   * @param page 需要合并的兄弟页
   * @param is_next 是否是后继页
   * @return PageCode
   */
  ResultCode MergePage(Page &sbling_page, bool is_next);

  /**
   * @brief 获取最后一天有效记录迭代器
   *
   * @return Iterator<Record>
   */
  Iterator<Record> GetLastIterator();

 public:
  bool full() const;
  uint16_t ValidDataSize() const;

 public:
  uint16_t alloc(uint16_t size) noexcept;
  uint16_t slot_offset(int pos) const noexcept;
  uint16_t slot(int slot_no, int record_no);
  void set_slot(int slot_no, uint16_t address) noexcept;

  /**
   * @brief 获取槽内记录的值
   *
   * @param slot_no 槽索引编号
   * @return uint16_t
   */
  uint16_t SlotValue(int slot_no) const noexcept;

  /**
   * @brief 插入槽
   *
   * @param slot_no 槽索引编号
   * @param value 值
   */
  void InsertSlot(int slot_no, uint16_t value) noexcept;

  /**
   * @brief 删除槽
   *
   * @param slot_no
   * @return uint16_t
   */
  uint16_t EraseSlot(int slot_no) noexcept;

  /**
   * @brief 定位目标key所在槽
   *
   * @param key 键值
   * @param compare 比较函数
   * @return uint16_t 对应的槽
   */
  uint16_t LocateSlot(string_view target_key,
                      const Compare &compare) const noexcept;

  /**
   * @brief 分裂对应槽, 平分槽内记录
   *
   * @param slot_no 槽索引编号
   */
  void SplitSlot(int slot_no) noexcept;

 public:
  /**
   * @brief 大于等于目标key的, 最小key
   *
   * @param key
   * @param compare
   * @return uint16_t
   */
  uint16_t LowerBound(string_view key, const Compare &compare) noexcept;
  /**
   * @brief 搜索小于目标key的,最大key
   *
   * @param key
   * @param compare 比较函数
   * @return uint16_t 返回记录地址
   */
  uint16_t FloorSearch(string_view target_key, const Compare &compare) noexcept;

 public:
  /**
   * @brief 定位记录的位置
   *
   * @param record_no 记录索引
   * @return uint16_t 记录地址
   */
  uint16_t LocateRecord(uint16_t record_no) noexcept;

  /**
   * @brief 定位记录的位置
   *
   * @param slot_no 槽索引
   * @param record_no 槽内记录索引
   * @return uint16_t 记录位置
   */
  uint16_t LocateRecord(uint16_t slot_no, uint16_t record_no) noexcept;

  /**
   * @brief 定位Page中的最后一条记录
   *
   * @return uint16_t
   */
  uint16_t LocateLastRecord() noexcept;

  /**
   * @brief 插入记录
   *
   * @param prev_record_address 插入记录的前驱节点
   * @param record_address 记录地址
   * @param key 键
   * @param val 键
   * @param compare 比较函数
   * @return uint16_t
   */
  uint16_t InsertRecord(uint16_t prev_record_address, uint16_t record_address,
                        string_view key, string_view val,
                        const Compare &compare);
  int set_record(uint16_t offset, const RecordMeta *meta,
                 const RecordData *record) noexcept;

 public:
  /**
   * @brief 获取迭代器
   *
   * @return Iterator<Record>
   */
  Iterator<Record> Iterator() noexcept;

 public:
  /**
   * @brief 填充数据
   *
   * @param offset 偏移量
   * @param data 数据
   * @return uint16_t 填充数据
   */
  uint16_t Fill(uint16_t offset, string_view data);

  /**
   * @brief 生成数据视图
   *
   * @param offset 偏移量
   * @param len 长度
   * @return string_view 视图
   */
  string_view View(uint16_t offset, size_t len);

  /**
   * @brief 整理页面
   *
   */
  void TidyPage();

  /**
   * @brief 拷贝页内记录到新页
   *
   * @param begin_record_address
   * @param end_record_address
   * @return Page
   */
  Page CopyRecordToNewPage(uint16_t begin_record_address,
                           uint16_t end_record_address);

  /**
   * @brief 尝试获取空间
   *
   * @param size
   * @return true
   * @return false
   */
  bool TryAlloc(uint16_t size) noexcept;

 public:
  void scan_use() noexcept;
  void scan_free() noexcept;
  void scan_slots() noexcept;
  void ScanData() noexcept;

 public:
  char *base_address() const noexcept;

 public:
  template <typename T>
  T *get_arribute(uint16_t offset);

  template <typename T>
  T *Attribute(uint16_t offset);

  template <typename T>
  const T *UnmodifiedAttribute(uint16_t offset) const;

  string_view Key(uint16_t record_address);
  string_view Value(uint16_t record_address);

  void move(uint16_t src_offset, uint16_t len, uint16_t dst_offset) noexcept;

 public:
  PageMeta *meta() { return meta_; }

 public:
  PageMeta *meta_;
  char *base_address_;

  shared_ptr<char> page_data_guard_;

  uint16_t virtual_min_record_address_;
  uint16_t virtual_max_record_address_;
  uint16_t last_record_address_;

 public:
  static uint16_t VIRTUAL_MIN_RECORD_SIZE;
};

template <typename T>
T *Page::get_arribute(uint16_t offset) {
  if (offset == 0) {
    return nullptr;
  }
  return reinterpret_cast<T *>(base_address_ + offset);
}

template <typename T>
T *Page::Attribute(uint16_t offset) {
  if (offset == 0) {
    return nullptr;
  }
  return reinterpret_cast<T *>(base_address_ + offset);
}

template <typename T>
const T *Page::UnmodifiedAttribute(uint16_t offset) const {
  if (offset == 0) {
    return nullptr;
  }
  return reinterpret_cast<T *>(base_address_ + offset);
}

template <>
struct FrameConstructor<Page> {
  template <typename... Args>
  static Page *Construct(Frame *frame, Args &&... args) {
    return new Page{std::forward<Args>(args)..., frame->buffer};
  }
};

template <>
struct FrameDeconstructor<Page> {
  static void Deconstruct(Page *data) {
    delete data;
    return;
  }
};

#endif