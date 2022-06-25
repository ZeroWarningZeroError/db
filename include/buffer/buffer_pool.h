#pragma once
#ifndef BUFFER_POOL_MANAGER
#define BUFFER_POOL_MANAGER

#include <list>
#include <unordered_map>
#include <vector>

#include "basetype.h"
#include "frame.h"
#include "io/SpaceManager.h"

using std::equal_to;
using std::hash;
using std::list;
using std::unordered_map;
using std::vector;

namespace std {
template <>
struct hash<PagePosition> {
  size_t operator()(const PagePosition& page_position) const {
    return hash<table_id_t>()(page_position.table_id) << 32 |
           hash<address_t>()(page_position.page_address);
  }
};

template <>
struct equal_to<PagePosition> {
  bool operator()(const PagePosition& lhs, const PagePosition& rhs) const {
    return lhs.table_id == rhs.table_id && lhs.page_address == rhs.page_address;
  }
};
};  // namespace std

class ISpaceManager;

class IBufferPool {
 public:
  virtual Frame* FetchPage(PagePosition page_position);
  virtual void PinPage(PagePosition page_position);
  virtual void UnPinPage(PagePosition page_position);
  virtual void FlushPage(PagePosition page_position);
  virtual Frame* NewPage(PagePosition page_position);
  virtual void DeletePage(PagePosition page_position);
  virtual void FlushAllPage();
};

class IReplacer;

class LRUBufferPool : public IBufferPool {
 public:
  LRUBufferPool(size_t capacity);
  LRUBufferPool(const LRUBufferPool& other) = delete;
  LRUBufferPool(const LRUBufferPool&& other) = delete;
  ~LRUBufferPool();

 public:
  virtual Frame* FetchPage(PagePosition page_position) override;
  virtual void PinPage(PagePosition page_position) override;
  virtual void UnPinPage(PagePosition page_position) override;
  virtual void FlushPage(PagePosition page_position) override;
  virtual Frame* NewPage(PagePosition page_position) override;
  virtual void DeletePage(PagePosition page_position) override;
  virtual void FlushAllPage();

 private:
  optional<frame_id_t> GetFreeFrame();
  Frame* NewEmptyFrame(frame_id_t frame_id);

 private:
  size_t capacity_;
  IReplacer* replacer;
  ISpaceManager* space_manager_;
  list<frame_id_t> frees_;
  vector<Frame*> frames_;
  unordered_map<PagePosition, frame_id_t> frame_ids_;
};

#endif