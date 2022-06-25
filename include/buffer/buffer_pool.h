#pragma once
#ifndef BUFFER_POOL_MANAGER
#define BUFFER_POOL_MANAGER

#include <list>
#include <optional>
#include <unordered_map>
#include <vector>

#include "basetype.h"
#include "frame.h"

using std::equal_to;
using std::hash;
using std::list;
using std::optional;
using std::unordered_map;
using std::vector;

class ISpaceManager;

class IBufferPool {
 public:
  virtual Frame* FetchPage(PagePosition page_position) = 0;
  virtual void UnPinPage(PagePosition page_position) = 0;
  virtual bool FlushPage(PagePosition page_position) = 0;
  // virtual void DeletePage(PagePosition page_position);
  virtual void FlushAllPage() = 0;
};

class IReplacer;
class ISpaceManager;

class LRUBufferPool : public IBufferPool {
 public:
  LRUBufferPool(size_t capacity);
  LRUBufferPool(const LRUBufferPool& other) = delete;
  LRUBufferPool(const LRUBufferPool&& other) = delete;
  ~LRUBufferPool();

 public:
  virtual Frame* FetchPage(PagePosition page_position) override;
  virtual void UnPinPage(PagePosition page_position) override;
  virtual bool FlushPage(PagePosition page_position) override;
  // virtual void DeletePage(PagePosition page_position) override;
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