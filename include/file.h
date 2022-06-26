#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

using std::cout;
using std::endl;
using std::fstream;
using std::lock_guard;
using std::mutex;
using std::string;

class File {
 public:
  File() = default;
  File(const std::string &db_file);
  File(const File &other) = delete;
  // File(const File &&other) = default;
  ~File();

 public:
  /**
   * @brief 文件是否存在
   *
   * @param file_path 文件路径
   * @return true
   * @return false
   */
  static bool exist(const std::string &file_path);

 public:
  /**
   * @brief 打开文件
   *
   * @param db_file 文件名
   * @return true  成功
   * @return false 失败
   */
  bool open(const std::string &db_file);

  /**
   * @brief 文件是否打开成功
   *
   * @return true  成功
   * @return false 失败
   */
  bool is_open() const;

  /**
   * @brief 从文件中申请指定大小的磁盘空间
   *
   * @param size 申请空间
   * @return size_t
   */
  size_t alloc(const size_t size);

  /**
   * @brief 从文件读取指定对象
   *
   * @tparam T 对象类型
   * @param pos 文件位置
   * @param obj 对象
   * @param size 读取大小
   */
  template <typename T>
  void read(int pos, T *obj, size_t size);

  /**
   * @brief 从文件中读取指定对象
   *
   * @tparam T
   * @param pos
   * @return T
   */
  template <typename T>
  T read(int pos);

  /**
   * @brief
   *
   * @param pos
   * @param buffer
   * @param size
   */
  void read(size_t pos, char *buffer, size_t size);

  /**
   * @brief 向文件写入数据
   *
   * @param pos
   * @param data
   * @param size
   */
  void write(size_t pos, const char *data, size_t size);

  /**
   * @brief 向文件中写入对象
   *
   * @tparam T 对象类型
   * @param pos 写入位置
   * @param obj 写入对象
   */
  template <typename T>
  void write(int pos, const T *obj, size_t size);

  /**
   * @brief 向文件尾部添加数据
   *
   * @param data 数据
   * @param len  数据长度
   */
  void append(const char *data, int len);

  /**
   * @brief 文件大小
   *
   */
  int64_t size();

 private:
  size_t last_pos();

 private:
  fstream handle_;
  string db_file_name_;
  mutex _file_lock;
  const size_t MARK = 1;
};

template <typename T>
void File::read(int pos, T *obj, size_t size) {
  lock_guard guard(_file_lock);
  handle_.seekg(pos);
  handle_.read(reinterpret_cast<char *>(obj), size);
}

template <typename T>
T File::read(int pos) {
  lock_guard guard(_file_lock);
  T obj;
  read(pos, &obj, sizeof(T));
  return obj;
}

template <typename T>
void File::write(int pos, const T *obj, size_t size) {
  if (size <= 0) {
    size = sizeof(T);
  }
  lock_guard guard(_file_lock);
  handle_.seekp(pos);
  handle_.write((char *)obj, size);
}