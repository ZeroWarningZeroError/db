#include "buffer_pool.h"

#include "replacer.h"

LRUBufferPool::LRUBufferPool(size_t capacity) : capacity_(capacity) {
  replacer = new LRUReplacer(capacity_);
  for (frame_id_t frame_id = 0; frame_id < capacity_; frame_id++) {
    frees_.emplace_back(frame_id);
    Frame* frame = new Frame{frame_id, 0, false, new char[PAGE_SIZE]};
    frames_.emplace_back(frame);
  }
}

Frame* LRUBufferPool::FetchPage(PagePosition page_position) {
  if (frame_ids_.count(page_position)) {
    frame_id_t frame_id = frame_ids_[page_position];
    auto frame = frames_[frame_id];
    replacer->Pin(frame_id);
    frame->pin_count++;
    return frame;
  }

  auto result = this->GetFreeFrame();
  if (result) {
    return nullptr;
  }
  frame_id_t frame_id = result.value();
  if (page_positions_.count(frame_id)) {
    auto old_page_position = page_positions_[frame_id];
    frame_ids_.erase(old_page_position);
  }

  frame_ids_[page_position] = frame_id;
  page_positions_[frame_id] = page_position;

  auto frame = frames_[frame_id];
  frame->dirty = false;
  frame->pin_count = 1;

  return frame;
}

void LRUBufferPool::UnPinPage(PagePosition page_position) {
  if (frame_ids_.count(page_positions) == 0) {
    return;
  }
  auto frame = frames_[frame_ids_[page_position]];
  frame->pin_count--;
  if (frame->pin_count <= 0) {
    replacer->Pin(frame->id);
  }
}

optional<frame_id_t> LRUBufferPool::GetFreeFrame() {
  if (frees_.size()) {
    auto frame_id = frees_.front();
    frees_.pop_front();
    return frame_id;
  }
  return replacer->Victim();
}