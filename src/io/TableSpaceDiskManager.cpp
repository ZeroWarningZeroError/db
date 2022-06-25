#include "io/TableSpaceDiskManager.h"

#include "file.h"

using std::lock_guard;

size_t TableSpaceDiskManager::read(space_t space, address_t address,
                                   char* buffer, size_t buffer_size) {
  File* file = getFile(space);
  file->read(address, buffer, buffer_size);
  return buffer_size;
}

size_t TableSpaceDiskManager::write(space_t space, address_t address,
                                    const char* buffer, size_t buffer_size) {
  File* file = getFile(space);
  file->write(address, buffer, buffer_size);
  return buffer_size;
}

File* TableSpaceDiskManager::getFile(space_t space) {
  lock_guard<mutex> guard(fetch_file_lock_);
  if (files_.count(space) == 0) {
    files_[space] = new File(space);
  }
  return files_[space];
}