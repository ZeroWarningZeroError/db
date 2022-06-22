#pragma once
#ifndef REPLACER_H
#define REPLACER_H

#include <mutex>
#include <optional>
#include <unordered_map>

#include "frame.h"

using std::lock_guard;
using std::mutex;
using std::optional;
using std::unordered_map;

class IReplacer {
 public:
  virtual optional<frame_id_t> Victim() = 0;
  virtual void Pin(frame_id_t frame_id);
  virtual void Unpin(frame_id_t frame_id);
  virtual size_t Size();
};

template <typename T>
struct LRUNode {
  LRUNode *next;
  LRUNode *prev;
  T data;
};

class LRUReplacer : public IReplacer {
 public:
  LRUReplacer(int size);
  LRUReplacer(const LRUReplacer &other) = delete;
  LRUReplacer(const LRUReplacer &&other) = delete;

 public:
  virtual optional<frame_id_t> Victim() override;
  virtual void Pin(frame_id_t frame_id) override;
  virtual void Unpin(frame_id_t frame_id) override;
  virtual size_t Size() override;

 private:
  size_t size_;
  size_t capacity_;
  LRUNode<frame_id_t> *head_;
  LRUNode<frame_id_t> *tail_;
  mutex lock_;
  unordered_map<frame_id_t, LRUNode<frame_id_t> *> nodes_;
};

#endif