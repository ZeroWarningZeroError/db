#pragma once
#include "buffer/buffer_pool.h"
#include "buffer/frame.h"

template <typename T>
struct FrameConstructor {
  template <typename... Args>
  static T *Construct(Frame *frame, Args... args) {
    T *obj = reinterpret_cast<T *>(frame->buffer);
    *obj = {args...};
    return obj;
  }
};
template <typename T>
struct FrameDeconstructor {
  static void Deconstruct(T *data) { return; }
};

template <typename T>
class LocalAutoReleaseFrameData {
 public:
  template <typename... Args>
  LocalAutoReleaseFrameData(IBufferPool *pool, PagePosition position,
                            Args... args) {
    pool_ = pool;
    position_ = position_;
    Frame *frame = pool_->FetchPage(position_);
    data_ = FrameConstructor<T>::Construct(frame, args...);
  }

  T *operator->() { return data_; }

  ~LocalAutoReleaseFrameData() {
    // pool_->FlushPage(position_);
    FrameDeconstructor<T>::Deconstruct(data_);
  }

 private:
  T *data_;
  PagePosition position_;
  IBufferPool *pool_;
};