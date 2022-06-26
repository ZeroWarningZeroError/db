#pragma once
#include "buffer/buffer_pool.h"
#include "buffer/frame.h"

// template <typename T>
// class LocalFrame {
//  public:
//   LocalFrame(IBufferPool *pool, Frame *frame, T *data) {
//     this->pool_ = pool;
//     this->frame_ = data;
//   }

//   LocalFrame(LocalFrame &) = delete;
//   LocalFrame(LocalFrame &&) = delete;

//   ~LocalFrame() { this->pool_->UnPinPage(frame_->page_position); }

//   T *operator->() { return reinterpret_cast<T *>(frame_->buffer); }

//  private:
//   IBufferPool *pool_;
//   Frame *frame_;
// };

// template <typename T>
// class SharedFrame {};