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
#include "io/TableSpaceDiskManager.h"
#include "memory/buffer.h"
#include "serialize.h"

using std::cout;
using std::endl;
using std::is_base_of_v;
using std::to_string;

int main() {
  IReplacer *replacer = new LRUReplacer(10);

  for (int i = 0; i < 10; i++) {
    replacer->Unpin(i);
  }

  replacer->scan();
  delete replacer;
}

// struct Test {
//   int a;
//   int b;
// };

// auto cmp = [](string_view v1, string_view v2) -> int {
//   if (v1 == v2) {
//     return 0;
//   }
//   return v1 < v2 ? 1 : -1;
// };

// string_view GetKey(Page *page, uint16_t record_address) {
//   RecordMeta *meta = page->get_arribute<RecordMeta>(record_address);
//   string_view key = {page->base_address() + record_address +
//   sizeof(RecordMeta),
//                      meta->key_len};
//   cout << (*meta) << " " << key << endl;
//   return key;
// }

// void Search(BPlusTreeIndex &index, string_view key) {
//   auto result = index.Search(key);
//   if (result.has_value()) {
//     cout << "key=" << key << ",value=" << result.value() << endl;
//   } else {
//     cout << "key no exist" << endl;
//   }
// }