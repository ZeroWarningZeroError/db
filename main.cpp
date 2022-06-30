#include <spdlog/spdlog.h>
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

struct PageMarker {
  static LocalAutoReleaseFrameData<Page> NewLocalBPlusTreePage(
      IBufferPool* pool, PageType page_type, PagePosition position) {
    return {pool, position, page_type, false};
  }

  static LocalAutoReleaseFrameData<Page> LoadLocalBPlustTreePage(
      IBufferPool* pool, PagePosition position) {
    return {pool, position, PageType::kLeafPage, true};
  }
};

int main() {
  IBufferPool* pool = new LRUBufferPool(10);
  {
    PagePosition p = {"index.data", 0};
    auto page =
        BufferedObjectMakerManager::Instance()->Select(pool)->NewObject<Page>(
            p, PageType::kLeafPage, true);

    page->Insert("key001", {"val001"}, cmp);
    page->Insert("key002", {"val001"}, cmp);
    page->Insert("key003", {"val001"}, cmp);
    page->Insert("key005", {"val001"}, cmp);
  }

  delete pool;
}