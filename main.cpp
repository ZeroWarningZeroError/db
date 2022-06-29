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

template <>
struct FrameConstructor<Page> {
  template <typename... Args>
  static Page* Construct(Frame* frame, Args... args) {
    Page* page = new Page{args..., frame->buffer};
    return page;
  }
};

template <>
struct FrameDeconstructor<Page> {
  static void Deconstruct(Page* data) { return; }
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
  auto data =
      PageMarker::NewLocalBPlusTreePage(pool, kLeafPage, {"data.index", 0});

  data->Insert("key0001", {"val0001"}, cmp);
  data->Insert("key0002", {"val0002"}, cmp);
  data->Insert("key0003", {"val0003"}, cmp);
  data->Insert("key0004", {"val0004"}, cmp);

  data->scan_use();

  spdlog::info("scan_use");

  delete pool;
}