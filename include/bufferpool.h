#pragma once

#include <cassert>
#include <iostream>
#include <mutex>
#include <unordered_map>

using std::cout;
using std::endl;
using std::mutex;
using std::unordered_map;

union BufferNode {
  union BufferNode *next;
  char current[0];
};

enum class BufferSize {
  BUFFER_1K = 1024,
  BUFFER_4K = 4 * BUFFER_1K,
  BUFFER_16K = 16 * BUFFER_4K
};

#define _ENUM_V(type) static_cast<size_t>(type)

class BufferPool {
public:
  struct BufferList {
    BufferSize size_type;
    BufferNode free;
    size_t free_size;
    size_t use_size;
  };

public:
  static BufferPool *instance();
  char *Alloc(BufferSize size);
  template <typename T> T *Alloc();
  template <typename T, typename... Args> T *Alloc(Args... args);
  template <typename T> void Release(const T *address);

public:
  ~BufferPool();

private:
  BufferPool();
  BufferPool(const BufferPool &other) = delete;
  BufferPool(const BufferPool &&other) = delete;

private:
  void ExpandBufferList(BufferList &list, size_t num);
  void ReleaseFreeBuffer(BufferList &list, size_t num);
  void ReleaseUseBuffer();
  void ReleaseBuffer(BufferNode &header, size_t num);

private:
  unordered_map<BufferSize, BufferList> _heads;
  unordered_map<const char *, BufferSize> _uses;
  mutex _lock;
};

template <typename T> T *BufferPool::Alloc() {
  static_assert(sizeof(T) <= _ENUM_V(BufferSize::BUFFER_16K));
  size_t obj_size = sizeof(T);
  if (obj_size <= _ENUM_V(BufferSize::BUFFER_1K)) {
    return reinterpret_cast<T *>(Alloc(BufferSize::BUFFER_1K));
  } else if (obj_size <= _ENUM_V(BufferSize::BUFFER_4K)) {
    return reinterpret_cast<T *>(Alloc(BufferSize::BUFFER_4K));
  } else {
    return reinterpret_cast<T *>(Alloc(BufferSize::BUFFER_16K));
  }
}

template <typename T, typename... Args> T *BufferPool::Alloc(Args... args) {
  T *obj = Alloc<T>();
  *obj = T{args...};
  return obj;
}

template <typename T> void BufferPool::Release(const T *obj) {
  // _lock.lock();
  auto address = reinterpret_cast<const char *>(obj);
  assert(_uses.count(address));
  auto &buffer_list = _heads[_uses[address]];
  auto node = reinterpret_cast<BufferNode *>(const_cast<char *>(address));
  node->next = buffer_list.free.next;
  buffer_list.free.next = node;
  // cout << (void*) buffer_list.free.next << endl;
  buffer_list.free_size++;
  buffer_list.use_size--;
  _uses.erase(address);
  // _lock.unlock();
}