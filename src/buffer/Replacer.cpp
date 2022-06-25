#include "buffer/replacer.h"

#include <iostream>

using std::cout;
using std::endl;
using std::nullopt;

#pragma region "LRUReplacer"

LRUReplacer::LRUReplacer(size_t capacity) : size_(0), capacity_(capacity) {
  head_ = new LRUNode<frame_id_t>;
  tail_ = new LRUNode<frame_id_t>;

  head_->next = tail_;
  head_->prev = tail_;
  tail_->prev = head_;
  tail_->next = head_;
}

LRUReplacer::~LRUReplacer() {
  auto cur = head_;
  do {
    auto next_node = cur->next;
    delete cur;
    cur = next_node;
  } while (cur != tail_);
}

optional<frame_id_t> LRUReplacer::Victim() {
  lock_guard<mutex> guard(lock_);
  if (head_->next == tail_) {
    return nullopt;
  }

  auto victim_lru_node = tail_->prev;
  auto prev_lru_node = victim_lru_node->prev;

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
  auto lru_node = nodes_[frame_id];
  auto prev_lru_node = lru_node->prev;
  auto next_lru_node = lru_node->next;
  prev_lru_node->next = next_lru_node;
  next_lru_node->prev = prev_lru_node;
  nodes_.erase(frame_id);
  size_--;
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  lock_guard<mutex> guard(lock_);

  LRUNode<frame_id_t> *lru_node;
  if (nodes_.count(frame_id)) {
    lru_node = nodes_[frame_id];
    auto prev_lru_node = lru_node->prev;
    auto next_lru_node = lru_node->next;
    prev_lru_node->next = next_lru_node;
    next_lru_node->prev = prev_lru_node;
  } else {
    lru_node = new LRUNode<frame_id_t>{nullptr, nullptr, frame_id};
    nodes_[frame_id] = lru_node;
    size_++;
  }

  auto head_next_node = head_->next;
  head_->next = lru_node;
  head_next_node->prev = lru_node;
  lru_node->prev = head_;
  lru_node->next = head_next_node;
}

size_t LRUReplacer::size() {
  lock_guard<mutex> guard(lock_);
  return size_;
}

void LRUReplacer::scan() {
  auto cur = head_->next;
  while (cur != tail_) {
    cout << cur->data << " ";
    cur = cur->next;
  }
  cout << endl;
}

#pragma endregion