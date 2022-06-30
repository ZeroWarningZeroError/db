#pragma once
#ifndef BUFFER_MAKER_H
#define BUFFER_MAKER_H

#include <mutex>
#include <unordered_map>

#include "buffer/extend_frame.h"

using std::lock_guard;
using std::mutex;
using std::unordered_map;

class BufferedObjectMaker {
 public:
  BufferedObjectMaker(IBufferPool *pool) : pool_(pool) {}
  template <typename T, typename... Args>
  SharedFrameData<T> NewObject(Args &&... args) {
    return {pool_, std::forward<Args>(args)...};
  }

 private:
  IBufferPool *pool_;
};

class BufferedObjectMakerManager {
 private:
  BufferedObjectMakerManager() = default;
  BufferedObjectMakerManager(const BufferedObjectMakerManager &) = delete;
  BufferedObjectMakerManager(const BufferedObjectMakerManager &&) = delete;

 public:
  static BufferedObjectMakerManager *Instance() {
    static BufferedObjectMakerManager manager;
    return &manager;
  }

 public:
  BufferedObjectMaker *Select(IBufferPool *pool) {
    lock_guard<mutex> lock(makers_mutex_);
    if (makers_.count(pool)) {
      return makers_[pool];
    }
    auto new_object_maker = new BufferedObjectMaker(pool);
    makers_[pool] = new_object_maker;
    return new_object_maker;
  }

 private:
  unordered_map<IBufferPool *, BufferedObjectMaker *> makers_;
  mutex makers_mutex_;
};

#endif
