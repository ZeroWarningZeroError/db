#ifndef REF_H
#define REF_H

#include <atomic>
#include <cstddef>

using std::atomic;

// todo 线程安全Reference

class Reference {
public:
  Reference() : reference_count_(new size_t(1)) {}
  Reference(const Reference &) = delete;
  Reference(const Reference &&) = delete;

  size_t DecrementReference() {
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

  size_t IncrementReference() { return ++(*(this->reference_count_)); }

  size_t reference_count() {
    if (reference_count_ == nullptr) {
      return 0;
    }
    return *reference_count_;
  }

private:
  size_t *reference_count_;
  // atomic<int>* reference_count_;
};

#endif