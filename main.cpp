#include <ctime>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

#include "bplustree.h"
#include "bplustree/page.h"
#include "file.h"
#include "fmt/format.h"
#include "memory/buffer.h"
#include "serialize.h"

using std::cout;
using std::endl;
using std::is_base_of_v;
using std::to_string;

struct Test {
  int a;
  int b;
};

string_view GetKey(Page *page, uint16_t record_address) {
  RecordMeta *meta = page->get_arribute<RecordMeta>(record_address);
  string_view key = {page->base_address() + record_address + sizeof(RecordMeta),
                     meta->key_len};
  cout << (*meta) << " " << key << endl;
  return key;
}

int main() {
  auto cmp = [](string_view v1, string_view v2) -> int {
    if (v1 == v2) {
      return 0;
    }
    return v1 < v2 ? 1 : -1;
  };

  BPlusTreeIndex index("t1.index", cmp);

  for (int i = 1000; i < 1016; i++) {
    auto code = index.Insert("key" + to_string(i), "val" + to_string(i));
  }

  index.BFS();

  cout << sizeof(RecordMeta) << endl;
  cout << sizeof(PageMeta) << endl;

  cout << (PAGE_SIZE - sizeof(PageMeta)) << endl;

  // Page page(kLeafPage);

  // for (int i = 0; i < 100; i++) {
  //   page.Insert("key" + to_string(i), {"val" + to_string(i)}, cmp);
  // }

  // cout << *page.meta() << endl;
  return 0;
}
