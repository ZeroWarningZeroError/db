#include <sys/stat.h>

#include <ctime>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

#include "bplustree.h"
#include "bplustree/bplustree_page.h"
#include "buffer/Replacer.h"
#include "buffer/buffer_pool.h"
#include "file.h"
#include "fmt/format.h"
#include "io/TableSpaceDiskManager.h"
#include "memory/buffer.h"
#include "serialize.h"

using std::cout;
using std::endl;
using std::is_base_of_v;
using std::to_string;

auto cmp = [](string_view v1, string_view v2) -> int {
  if (v1 == v2) {
    return 0;
  }
  return v1 < v2 ? 1 : -1;
};

int main() {
  IBufferPool* pool = new LRUBufferPool(10);
  Frame* frame = pool->FetchPage({"t1.index", 10});
  for (int i = 0; i < frame->frame_size; i++) {
    cout << frame->buffer[i];
  }
  cout << endl;

  for (int i = 0; i < frame->frame_size; i++) {
    frame->buffer[i] = 'a' + (i % 26);
  }

  frame->is_dirty = true;

  pool->FlushPage(frame->page_position);
  pool->UnPinPage(frame->page_position);

  delete pool;
  return 0;
}