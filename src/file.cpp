#include "file.h"

#include <spdlog/spdlog.h>
#include <sys/stat.h>

#include <algorithm>
#include <filesystem>

using std::min;

bool File::exist(const std::string &file_path) {
  return std::filesystem::exists(file_path);
}

File::File(const std::string &db_file) : db_file_name_(db_file) {
  open(db_file);
}

bool File::open(const std::string &db_file) {
  db_file_name_ = db_file;
  handle_.open(db_file, std::ios::binary | std::ios::in | std::ios::out);
  if (!handle_.is_open()) {
    handle_.open(db_file, std::ios::binary | std::ios::app);
    handle_.close();
    handle_.open(db_file, std::ios::binary | std::ios::in | std::ios::out);
  }
  return handle_.is_open();
}

bool File::is_open() const { return handle_.is_open(); }

void File::read(size_t pos, char *buffer, size_t size) {
  lock_guard guard(_file_lock);
  auto max_offset = this->size();
  if (max_offset <= pos) {
    spdlog::error("{}: file={}, pos={}, size={}, maxoffset={}", __func__,
                  this->db_file_name_, pos, size, max_offset);
    return;
  }
  auto real_size = min(max_offset - pos, size);
  if (real_size <= 0) {
    return;
  }
  handle_.seekg(pos);
  handle_.read(buffer, real_size);
  if (handle_.bad()) {
    spdlog::error("{}: file={}, pos={}, size={}, bad={}", __func__,
                  this->db_file_name_, pos, size, handle_.bad());
    return;
  }
  spdlog::info("{}: file={}, pos={}, size={}", __func__, this->db_file_name_,
               pos, size);
}

size_t File::alloc(const size_t size) {
  lock_guard guard(_file_lock);
  int last = last_pos();
  handle_.seekp(last + size);
  handle_.write((char *)&MARK, sizeof(size_t));
  return last;
}

size_t File::last_pos() {
  handle_.seekp(0, std::ios::end);
  return handle_.tellp();
}

void File::write(size_t pos, const char *data, size_t len) {
  lock_guard guard(_file_lock);
  handle_.seekp(pos);
  handle_.write(data, len);
  if (handle_.bad()) {
    spdlog::error("{}: file={}, pos={}, size={}, bad={}", __func__,
                  this->db_file_name_, pos, len, handle_.bad());
    return;
  }
  spdlog::info("{}: file={}, pos={}, size={}", __func__, this->db_file_name_,
               pos, len);
}

void File::append(const char *data, int len) {
  handle_.seekp(0, std::ios::end);
  handle_.write(data, len);
}

int64_t File::size() {
  struct stat buf;
  if (stat(db_file_name_.c_str(), &buf) == 0) {
    return buf.st_size;
  }
  return -1;
}

File::~File() {
  lock_guard guard(_file_lock);
  cout << "file close" << endl;
  handle_.flush();
  handle_.close();
}
