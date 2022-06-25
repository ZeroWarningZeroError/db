#ifndef FRAME_H
#define FRAME_H

#include <type_traits>

using std::hash;

struct PagePosition {
  space_t space;
  address_t page_address;
};

struct Frame {
  PagePosition page_position;
  frame_id_t id;
  space_t space;
  address_t page_address;
  uint64_t pin_count;
  char* buffer;
  bool is_dirty;
};

namespace std {
template <>
struct hash<Frame> {
  size_t operator()(const Frame& frame) const noexcept {
    return std::hash<frame_id_t>()(frame.id);
  }
};
}  // namespace  std

#endif