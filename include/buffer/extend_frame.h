#pragma once
#include <spdlog/spdlog.h>

#include <utility>

#include "buffer/buffer_pool.h"
#include "buffer/construct.h"
#include "buffer/frame.h"
#include "memory/ref.h"

using std::move;

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
  SharedFrameData() {
    pool_ = nullptr;
    frame_ = nullptr;
    data_ = nullptr;
  };

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

  SharedFrameData<T> &operator=(SharedFrameData<T> &&other) {
    this->DecrementReference();
    if (this->reference_count() == 0) {
      this->Deconstruct();
    }
    return this->Move(move(other));
  }

  void Deconstruct() {
    if (pool_ != nullptr) {
      pool_->UnPinPage(position_);
      FrameDeconstructor<T>::Deconstruct(data_);
    }
  }

  ~SharedFrameData() {
    this->DecrementReference();
    if (this->reference_count() == 0) {
      this->Deconstruct();
    }
  }

 public:
  const T *operator->() const { return data_; }
  // T *operator->() const { return data_; }
  Frame *frame() { return frame_; }
  T *data() { return data_; }

 private:
  SharedFrameData<T> &Copy(const SharedFrameData<T> &other) {
    this->IncrementReference();
    this->reference_count_ = other.reference_count_;
    this->pool_ = other.pool_;
    this->position_ = other.position_;
    this->data_ = other.data_;
    return *this;
  }

  SharedFrameData<T> &Move(SharedFrameData<T> &&other) {
    this->reference_count_ = move(other.reference_count_);
    this->pool_ = move(other.pool_);
    this->position_ = move(other.position_);
    this->data_ = move(other.data_);
    return *this;
  }

 private:
  IBufferPool *pool_;
  Frame *frame_;
  T *data_;
  PagePosition position_;
};