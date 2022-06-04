#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>

#include "base.h"

using std::cout;
using std::enable_if_t;
using std::endl;
using std::is_base_of_v;
using std::move;

template <typename T> class SharedBuffer {
public:
  template <typename... Args>
  SharedBuffer(size_t buffer_size = sizeof(T), Args... args)
      : buffer_size_(buffer_size) {
    cout << "SharedBuffer" << endl;
    reference_count_ = new size_t(1);
    buffer_ = new char[buffer_size_];
    memset(buffer_, 0, buffer_size_);
    Constructor<T>::construct(buffer_, args...);
  }

  template <typename... Args> void Reset(size_t buffer_size, Args... args) {
    DecrementReference();
    buffer_size_ = buffer_size;
    reference_count_ = new size_t(1);
    buffer_ = new char[buffer_size_];
    Constructor<T>::construct(buffer_, args...);
  }

  SharedBuffer(const SharedBuffer &other) {
    this->reference_count_ = other.reference_count_;
    this->buffer_ = other.buffer_;
    this->IncrementReference();
  }

  SharedBuffer &operator=(const SharedBuffer &other) {
    // 原始引用减一
    this->DecrementReference();
    this->reference_count_ = other.reference_count_;
    this->buffer_ = other.buffer_;
    this->IncrementReference();
    cout << "copy assign: reference_count:" << *(this->reference_count_)
         << endl;
    return *this;
  }

  SharedBuffer(const SharedBuffer &&other) {
    this->DecrementReference();
    this->reference_count_ = other.reference_count_;
    this->buffer_ = other.buffer_;
  }

  SharedBuffer &operator=(const SharedBuffer &&other) {
    this->DecrementReference();
    this->reference_count_ = other.reference_count_;
    this->buffer_ = other.buffer_;
    return *this;
  }

  T *operator->() { return reinterpret_cast<T *>(this->buffer_); }

  template <typename U, typename = enable_if_t<is_base_of_v<T, U>>>
  operator U *() {
    return reinterpret_cast<U *>(this->buffer_);
  }

  ~SharedBuffer() { DecrementReference(); }

  int reference_count() { return *(this->reference_count_); }

  char *buffer() { return buffer_; }

public:
  void DecrementReference() {
    if (reference_count_ != nullptr) {
      --(*reference_count_);
      cout << "Delete:" << (*reference_count_) << endl;
      if (*reference_count_ == 0) {
        cout << "Delete" << endl;
        delete reference_count_;
        delete[] buffer_;
        reference_count_ = nullptr;
        buffer_ = nullptr;
      }
    }
  }

  void IncrementReference() { ++(*(this->reference_count_)); }

private:
  size_t buffer_size_;
  size_t *reference_count_;
  char *buffer_;
};

#endif