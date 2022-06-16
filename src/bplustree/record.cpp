#include "bplustree/record.h"

#include <fmt/format.h>

#include <cstring>

#include "bplustree/page.h"

ostream &operator<<(ostream &os, const RecordMeta &meta) {
  return os << fmt::format(
             "[next={}, owned={}, delete_mask={}, len={}, "
             "key_len={}, val_len={}, slot_no={}]",
             meta.next, meta.owned, meta.delete_mask, meta.len, meta.key_len,
             meta.val_len, meta.slot_no);
}

ostream &operator<<(ostream &os, const Record &record) {
  return os << "[key=" << record.key << ", val=" << record.val << "]";
}

Iterator<Record>::Iterator(Page *page, uint16_t record_address) {
  page_ = page;
  address_ = record_address;

  if (address_ == 0) {
    return;
  }

  auto meta_ = page_->get_arribute<RecordMeta>(record_address);
  memcpy(&record_, meta_, sizeof(RecordMeta));
  record_.key = {page_->base_address() + record_address + sizeof(RecordMeta),
                 static_cast<size_t>(meta_->key_len)};
  record_.val = {page_->base_address() + record_address + sizeof(RecordMeta) +
                     meta_->key_len,
                 static_cast<size_t>(meta_->len - meta_->key_len)};
}

bool Iterator<Record>::isVirtualRecord() {
  return address_ == page_->virtual_min_record_address_ ||
         address_ == page_->virtual_max_record_address_;
}