#include <ctime>
#include <iostream>
#include <string_view>

#include "bplustree.h"
#include "bplustree/page.h"
#include "file.h"
#include "fmt/format.h"
#include "memory/buffer.h"
#include "serialize.h"
#include <string>
#include <type_traits>

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

  // string data = Serializer<uint16_t>::serialize(10299);
  // cout << data.size() << endl;
  // cout << Serializer<uint16_t>::deserialize(data) << endl;

  auto cmp = [](string_view v1, string_view v2) -> int {
    if (v1 == v2) {
      return 0;
    }
    return v1 < v2 ? 1 : -1;
  };

  // Page* page = new Page(kInternalPage);

  // page->Insert("5", { Serializer<address_t>::serialize(97),
  // Serializer<address_t>::serialize(120)}, cmp); page->Insert("4", {
  // Serializer<address_t>::serialize(97),
  // Serializer<address_t>::serialize(98)}, cmp); page->Insert("6", {
  // Serializer<address_t>::serialize(120),
  // Serializer<address_t>::serialize(121)}, cmp);

  // auto [mid_key, other_page] = page->SplitPage();

  Page *page = new Page(kLeafPage);

  page->Insert("5", {Serializer<address_t>::serialize(97)}, cmp);
  page->Insert("4", {Serializer<address_t>::serialize(97)}, cmp);
  page->Insert("6", {Serializer<address_t>::serialize(120)}, cmp);

  for (int i = 0; i < 20; i++) {
    page->Insert("key" + to_string(i), {"val" + to_string(i)}, cmp);
  }

  auto [mid_key, other_page] = page->SplitPage();

  page->scan_use();
  page->scan_slots();
  other_page.scan_use();
  other_page.scan_slots();

  // page_->scan_slots();
  // GetKey(page_, page_->FloorSearch("key_test", cmp));
  // cout << page_->LocateSlot("key_test" + to_string(48), cmp) << endl;

  return 0;
}
