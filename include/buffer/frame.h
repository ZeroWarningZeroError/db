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
  uint64_t frame_size;
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

namespace std {
template <>
struct hash<PagePosition> {
  size_t operator()(const PagePosition& page_position) const {
    return hash<space_t>()(page_position.space) << 32 |
           hash<address_t>()(page_position.page_address);
  }
};

template <>
struct equal_to<PagePosition> {
  bool operator()(const PagePosition& lhs, const PagePosition& rhs) const {
    return lhs.space == rhs.space && lhs.page_address == rhs.page_address;
  }
};
};  // namespace std

#endif