#pragma once
#include <spdlog/spdlog.h>

#include "buffer/buffer_pool.h"
#include "buffer/construct.h"
#include "buffer/frame.h"
#include "memory/ref.h"

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
    pool_->FlushPage(position_);
    FrameDeconstructor<T>::Deconstruct(data_);
  }

 private:
  T *data_;
  PagePosition position_;
  IBufferPool *pool_;
};

template <typename T>
class SharedFrameData : public Reference {
 public:
  template <typename... Args>
  SharedFrameData(IBufferPool *pool, PagePosition position, Args &&... args) {
    pool_ = pool;
    position_ = position;
    frame_ = pool_->FetchPage(position_);
    data_ = FrameConstructor<T>::Construct(frame_, std::forward<Args>(args)...);
  }

  SharedFrameData(const SharedFrameData<T> &other) { this->Copy(other); }

  SharedFrameData(const SharedFrameData<T> &&other) { this->Move(move(other)); }

  SharedFrameData<T> &operator=(const SharedFrameData<T> &other) {
    this->DecrementReference();
    if (this->reference_count() == 0) {
      this->Deconstruct();
    }
    return this->Copy(other);
  }

  SharedFrameData<T> &operator=(const SharedFrameData<T> &&other) {
    this->DecrementReference();
    if (this->reference_count() == 0) {
      this->Deconstruct();
    }
    return this->Move(move(other));
  }

  void Deconstruct() {
    spdlog::info("flush:space={},address={}", position_.space,
                 position_.page_address);
    pool_->UnPinPage(position_);
    FrameDeconstructor<T>::Deconstruct(data_);
  }

  ~SharedFrameData() {
    this->DecrementReference();
    if (this->reference_count() == 0) {
      this->Deconstruct();
    }
  }

 public:
  T *operator->() {
    frame_->is_dirty = true;
    return data_;
  }
  const T *operator->() const { return data_; }
  Frame *frame() { return frame_; }
  T *data() { return data_; }

 private:
  SharedFrameData<T> Copy(const SharedFrameData<T> &other) {
    this->IncrementReference();
    this->reference_count_ = other.reference_count_;
    this->pool_ = other.pool_;
    this->position_ = other.position_;
    this->data_ = other.data_;
  }

  SharedFrameData<T> Move(const SharedFrameData<T> &&other) {
    this->reference_count_ = move(other.reference_count_);
    this->pool_ = move(other.pool_);
    this->position_ = move(other.position_);
    this->data_ = move(other.data_);
  }

 private:
  IBufferPool *pool_;
  Frame *frame_;
  T *data_;
  PagePosition position_;
};