#include "Replacer.h"

using std::nullopt;

#pragma region "LRUReplacer"

LRUReplacer::LRUReplacer(size_t capacity) : size_(0), capacity_(capacity) {
  head_->next = tail_;
  head_->prev = tail_;
  tail_->prev = head_;
  tail_->next = head_;
}

optional<frame_id_t> LRUReplacer::Victim() {
  lock_guard<mutex> guard(lock_);
  if (head_->next == tail_) {
    return nullopt;
  }

  auto victim_lru_node = tail_->prev;
  auto prev_lru_node = victim_node->prev;

  prev_lru_node->next = tail_;
  tail_->prev = prev_lru_node;

  nodes_.erase(victim_lru_node->data);
  size_--;
  return victim_lru_node->data;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  lock_guard<mutex> guard(lock_);

  if (nodes_.count(frame_id) == 0) {
    return;
  }
  auto lru_node = nodes[frame_id];
  auto prev_lru_node = lru_node->prev;
  auto next_lru_node = lru_node->next;
  prev_lru_node->next = next_lru_node;
  next_lru_node->prev = prev_lru_node;
  nodes_->erase(frame_id);
  size_--;
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  lock_guard<mutex> guard(lock_);

  LRUNode<frame_id_t> *lru_node;
  if (node_.count(frame_id)) {
    lru_node = nodes[frame_id];
    auto prev_lru_node = lru_node->prev;
    auto next_lru_node = lru_node->next;
    prev_lru_node->next = next_lru_node;
    next_lru_node->prev = prev_lru_node;
  } else {
    lru_node = new LRUNode{nullptr, nullptr, frame_id};
    size_++;
  }

  auto head_next_node = head_->next;
  head_->next = lru_node;
  head_next_node->prev = lru_node;
  lru_node->prev = nullptr;
  lru_node->next = nullptr;
}

size_t LRUReplacer::Size() {
  lock_guard<mutex> guard(lock_);
  return size_;
}

#pragma endregion