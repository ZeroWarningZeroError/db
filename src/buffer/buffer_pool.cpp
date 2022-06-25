#include "buffer_pool.h"

#include "SpaceManager.h"
#include "TableSpaceDiskManager.h"
#include "replacer.h"

LRUBufferPool::LRUBufferPool(size_t capacity) : capacity_(capacity) {
  replacer = new LRUReplacer(capacity_);
  space_manager_ = TableSpaceDiskManager::instance();

  for (frame_id_t frame_id = 0; frame_id < capacity_; frame_id++) {
    frees_.emplace_back(frame_id);
    frames_.emplace_back(this->NewEmptyFrame(frame_id));
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
  auto frame = frames_[frame_id];

  frames_.erase(frame->page_position);
  frames_[page_position] = page_position;

  frame->dirty = false;
  frame->pin_count = 1;

  space_manager_->read(page_position.space, page_position.page_address,
                       frame->buffer, PAGE_SIZE);

  return frame;
}

void LRUBufferPool::UnPinPage(PagePosition page_position) {
  if (frame_ids_.count(page_positions) == 0) {
    return;
  }
  auto frame = frames_[frame_ids_[page_position]];
  frame->pin_count--;
  if (frame->pin_count <= 0) {
    replacer->UnPin(frame->id);
  }
}

bool LRUBufferPool::FlushPage(PagePosition page_position) {
  if (frame_ids_.count(page_position)) {
    return false;
  }
  auto frame_id = frame_ids_[page_position];
  auto frame = &frames_[frame_id];

  // 回刷
  space_manager_->write(page_position.space, page_position.page_address,
                        frame.buffer, PAGE_SIZE);

  return true;
}

virtual Frame* LRUBufferPool::DeletePage(PagePosition page_position) {}

virtual void LRUBufferPool::FlushAllPage() {}

Frame* LRUBufferPool::NewPage(PagePosition page_position) {
  if (frame_ids_.count(page_position)) {
    return frame_ids_[page_position];
  }

  auto result = this->GetFreeFrame();
  if (!result) {
    return nullptr;
  }

  frame_id_t frame_id = result.value();

  auto frame = &frames_[frame_id];
  if (frame->is_dirty) {
    space_manager_->write(page_position.space, page_position.page_address,
                          frame.buffer, PAGE_SIZE);
  }

  frame_ids_.erase(frame->page_position);
  frame_ids_[page_position] = frame_id;
  frame->page_position = page_position;

  space_manager_->read(page_position.space, page_position.page_address,
                       frame.buffer, PAGE_SIZE);

  return frame;
}

optional<frame_id_t> LRUBufferPool::GetFreeFrame() {
  if (frees_.size()) {
    auto frame_id = frees_.front();
    frees_.pop_front();
    return frame_id;
  }
  return replacer->Victim();
}

Frame* LRUBufferPool::NewEmptyFrame(frame_id_t frame_id) {
  Frame* frame = new Frame();
  frame->id = frame_id;
  frame->is_dirty = false;
  frame->page_position = {0, 0};
  frame->pin_count = 0;
  frame->buffer = new char[PAGE_SIZE];
  return frame;
}