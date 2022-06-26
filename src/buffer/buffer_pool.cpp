#include "buffer/buffer_pool.h"

#include <iostream>

#include "buffer/replacer.h"
#include "io/SpaceManager.h"
#include "io/TableSpaceDiskManager.h"

using std::cout;
using std::endl;
using std::lock_guard;

LRUBufferPool::LRUBufferPool(size_t capacity) : capacity_(capacity) {
  replacer = new LRUReplacer(capacity_);
  space_manager_ = TableSpaceDiskManager::instance();

  for (frame_id_t frame_id = 0; frame_id < capacity_; frame_id++) {
    frees_.emplace_back(frame_id);
    frames_.emplace_back(this->NewEmptyFrame(frame_id));
  }
}

LRUBufferPool::~LRUBufferPool() {
  this->FlushAllPage();
  for (int i = 0; i < capacity_; i++) {
    delete frames_[i]->buffer;
    delete frames_[i];
  }
  delete replacer;
}

Frame* LRUBufferPool::FetchPage(PagePosition page_position) {
  lock_guard<mutex> guard(pool_lock_);
  if (frame_ids_.count(page_position)) {
    auto frame_id = frame_ids_[page_position];
    auto frame = frames_[frame_id];
    frame->pin_count++;
    replacer->Pin(frame_id);
    return frame;
  }

  auto result = this->GetFreeFrame();
  if (!result.has_value()) {
    return nullptr;
  }

  frame_id_t frame_id = result.value();
  auto frame = frames_[frame_id];

  if (frame->is_dirty) {
    space_manager_->write(frame->page_position.space,
                          frame->page_position.page_address, frame->buffer,
                          PAGE_SIZE);
  }

  frame_ids_.erase(frame->page_position);
  frame_ids_[page_position] = frame_id;

  frame->is_dirty = false;
  frame->pin_count = 1;
  frame->page_position = page_position;

  space_manager_->read(page_position.space, page_position.page_address,
                       frame->buffer, PAGE_SIZE);

  return frame;
}

void LRUBufferPool::UnPinPage(PagePosition page_position) {
  lock_guard<mutex> guard(pool_lock_);

  if (frame_ids_.count(page_position) == 0) {
    return;
  }
  auto frame = frames_[frame_ids_[page_position]];
  frame->pin_count--;
  if (frame->pin_count <= 0) {
    replacer->Unpin(frame->id);
  }
}

bool LRUBufferPool::FlushPage(PagePosition page_position) {
  lock_guard<mutex> guard(pool_lock_);

  if (frame_ids_.count(page_position) == 0) {
    cout << "false" << endl;
    return false;
  }
  auto frame_id = frame_ids_[page_position];
  auto frame = frames_[frame_id];

  if (frame->is_dirty) {
    space_manager_->write(page_position.space, page_position.page_address,
                          frame->buffer, PAGE_SIZE);
    frame->is_dirty = false;
  }

  return true;
}

void LRUBufferPool::FlushAllPage() {
  lock_guard<mutex> guard(pool_lock_);

  for (int i = 0; i < capacity_; i++) {
    if (frames_[i]->is_dirty) {
      auto page_position = frames_[i]->page_position;
      space_manager_->write(page_position.space, page_position.page_address,
                            frames_[i]->buffer, PAGE_SIZE);
    }
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

Frame* LRUBufferPool::NewEmptyFrame(frame_id_t frame_id) {
  Frame* frame = new Frame();
  frame->id = frame_id;
  frame->is_dirty = false;
  frame->pin_count = 0;
  frame->buffer = new char[PAGE_SIZE];
  frame->frame_size = PAGE_SIZE;
  memset(frame->buffer, 0, PAGE_SIZE);
  return frame;
}