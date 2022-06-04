#include <fmt/format.h>

#include "bplustree/record.h"

ostream &operator<<(ostream &os, const RecordMeta &meta) {
  return os << fmt::format("[next={}, owned={}, delete_mask={}, len={}, "
                           "key_len={}, val_len={}, slot_no={}]",
                           meta.next, meta.owned, meta.delete_mask, meta.len,
                           meta.key_len, meta.val_len, meta.slot_no);
}

ostream &operator<<(ostream &os, const Record &record) {
  return os << "[key=" << record.key << ", val=" << record.val << "]";
}