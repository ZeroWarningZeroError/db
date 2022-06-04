#include <iostream>
#include <mutex>

#include "bufferpool.h"

using std::cout;
using std::endl;
using std::lock_guard;

BufferPool *BufferPool::instance() {
  static BufferPool _inner_instance;
  return &_inner_instance;
}

BufferPool::BufferPool() {
  _heads.insert(
      {BufferSize::BUFFER_1K, {BufferSize::BUFFER_1K, {nullptr}, 0, 0}});
  _heads.insert(
      {BufferSize::BUFFER_4K, {BufferSize::BUFFER_4K, {nullptr}, 0, 0}});
  _heads.insert(
      {BufferSize::BUFFER_16K, {BufferSize::BUFFER_4K, {nullptr}, 0, 0}});
}

char *BufferPool::Alloc(BufferSize buffer_size) {
  BufferList &buffer_list = _heads[buffer_size];
  // lock_guard guard(_lock);
  if (buffer_list.free_size == 0) {
    ExpandBufferList(buffer_list, buffer_list.use_size / 2 + 2);
  }
  BufferNode &free_head = buffer_list.free;
  BufferNode *free = free_head.next;
  free_head.next = free->next;
  buffer_list.free_size--;
  buffer_list.use_size++;
  // cout << "alloc:address=" << (void*)free->current << endl;
  _uses[reinterpret_cast<char *>(free)] = buffer_size;
  return free->current;
}

void BufferPool::ExpandBufferList(BufferList &buffer_list, size_t num) {
  // cout << "Expand:num=" << num << endl;
  for (int i = 0; i < num; i++) {
    BufferNode *cur = reinterpret_cast<BufferNode *>(
        new char[static_cast<size_t>(buffer_list.size_type)]);
    cur->next = buffer_list.free.next;
    buffer_list.free.next = cur;
  }
  buffer_list.free_size += num;
}

void BufferPool::ReleaseFreeBuffer(BufferList &buffer_list, size_t num) {
  if (num == -1 || num > buffer_list.free_size) {
    num = buffer_list.free_size;
  }

  ReleaseBuffer(buffer_list.free, num);
}

void BufferPool::ReleaseUseBuffer() {
  for (auto &[use, _] : _uses) {
    // cout << "delete:address=" << (void*)use << endl;
    delete use;
  }
}

void BufferPool::ReleaseBuffer(BufferNode &header, size_t num) {
  auto cur = header.next;
  while (num > 0 || cur) {
    auto next = cur->next;
    // cout << "delete:address=" << (void*)cur->current << endl;
    delete cur;
    cur = next;
    num--;
  }
}

BufferPool::~BufferPool() {
  for (auto &[_, buffer_list] : _heads) {
    ReleaseFreeBuffer(buffer_list, -1);
  }
  ReleaseUseBuffer();
}