#include <ctime>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

#include "bplustree.h"
#include "bplustree/page.h"
#include "buffer/Replacer.h"
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

auto cmp = [](string_view v1, string_view v2) -> int {
  if (v1 == v2) {
    return 0;
  }
  return v1 < v2 ? 1 : -1;
};

string_view GetKey(Page *page, uint16_t record_address) {
  RecordMeta *meta = page->get_arribute<RecordMeta>(record_address);
  string_view key = {page->base_address() + record_address + sizeof(RecordMeta),
                     meta->key_len};
  cout << (*meta) << " " << key << endl;
  return key;
}

void Search(BPlusTreeIndex &index, string_view key) {
  auto result = index.Search(key);
  if (result.has_value()) {
    cout << "key=" << key << ",value=" << result.value() << endl;
  } else {
    cout << "key no exist" << endl;
  }
}

int main() {
  BPlusTreeIndex index("t1.index", cmp);

  for (int i = 1000; i < 1020; i++) {
    auto code = index.Insert("key" + to_string(i), "val" + to_string(i));
  }

  index.BFS();

  Search(index, "key1000");
  Search(index, "key1010");
  Search(index, "key1017");
  Search(index, "key2222222");
  Search(index, "key0000");

  for (int i = 1000; i < 1002; i++) {
    for (int j = i; j < 1020; j += 3) {
      auto code = index.Erase("key" + to_string(j));
    }
    index.BFS();
  }
  return 0;
}
