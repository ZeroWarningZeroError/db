#ifndef FRAME_H
#define FRAME_H

#include <type_traits>

using std::hash;

using frame_id_t = int64_t;

struct Frame {
  frame_id_t id;
  int pin_count;
  bool is_dirty;
  char* buffer;
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