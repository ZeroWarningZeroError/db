#ifndef FIXED_ALLOCATOR_H
#define FIXED_ALLOCATOR_H

#include <atomic>
#include <cstddef>
#include <vector>

using std::atomic;
using std::vector;

template <size_t MEMORY_SIZE> class FixedAllocator {
public:
  FixedAllocator(size_t initial_capacity = 10)
      : buffers_(10), alloc_pos_(0), dealloc_pos_(0) {}

  char *Alloc() {}

  void Dealloc() {}

public:
  static const int DEFAULT_INITIAL_CAPACITY = 0;

private:
  vector<char *> buffers_;
  atomic<uint64_t> alloc_pos_;
  atomic<uint64_t> dealloc_pos_;
};

#endif