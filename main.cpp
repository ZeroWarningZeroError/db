#include <spdlog/spdlog.h>
#include <sys/stat.h>

#include <ctime>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

#include "bplustree/bplustree.h"
#include "bplustree/bplustree_page.h"
#include "buffer/Replacer.h"
#include "buffer/buffer_pool.h"
#include "buffer/extend_frame.h"
#include "buffer/maker.h"
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

struct Test {
  int a;
  int b;
  int c;
};

int main() {
  LRUBufferPool pool(10);
  PagePosition p = {"data2.index", 0};

  auto test =
      BufferedObjectMakerManager::Instance()->Select(&pool)->NewObject<Test>(
          PagePosition{"data2.index", 0}, 0, 1, 2);

  cout << test->a << endl;
  // test->a = 1;
  // cout << test->a << endl;
  // // BPlusTreeIndex index("test.db", cmp, &pool);
  // auto page =
  //     BufferedObjectMakerManager::Instance()->Select(&pool)->NewObject<Page>(
  //         p, PageType::kLeafPage, true);

  // SharedFrameData<T>
  // T*
  // page->scan_use();

  // IBufferPool *pool = new LRUBufferPool(10);
  // new LFR
  // pool = new LFUBufferPool(10);
  // pool->FetchPage();

  return 0;
}