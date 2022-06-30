#ifndef REF_H
#define REF_H

#include <atomic>
#include <cstddef>
#include <mutex>

using std::lock_guard;
using std::mutex;

// todo 线程安全Reference

class Reference {
 public:
  Reference() : reference_count_(new size_t(1)) {}
  Reference(const Reference &) = delete;
  Reference(const Reference &&) = delete;

  size_t DecrementReference() {
    lock_guard<mutex> lock(reference_mutex_);
    if (reference_count_ != nullptr) {
      --(*reference_count_);
      if (*reference_count_ == 0) {
        delete reference_count_;
        reference_count_ = nullptr;
        return 0;
      }
    }
    return *reference_count_;
  }

  size_t IncrementReference() {
    lock_guard<mutex> lock(reference_mutex_);
    return ++(*(this->reference_count_));
  }

  size_t reference_count() {
    lock_guard<mutex> lock(reference_mutex_);
    if (reference_count_ == nullptr) {
      return 0;
    }
    return *reference_count_;
  }

 private:
  size_t *reference_count_;
  mutex reference_mutex_;
};

#endif