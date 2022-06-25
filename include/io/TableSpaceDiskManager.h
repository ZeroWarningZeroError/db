#pragma once

#include <mutex>
#include <unordered_map>

#include "basetype.h"
#include "io/SpaceManager.h"

using std::mutex;

class File;

using std::unordered_map;

#define STATIC_SINGLE_INSTANCE(type) \
  static type* instance() {          \
    static type obj;                 \
    return &obj;                     \
  }

class TableSpaceDiskManager : public ISpaceManager {
 private:
  TableSpaceDiskManager() = default;
  TableSpaceDiskManager(const TableSpaceDiskManager& other) = delete;
  TableSpaceDiskManager(const TableSpaceDiskManager&& other) = delete;

 public:
  STATIC_SINGLE_INSTANCE(TableSpaceDiskManager);

 public:
  size_t read(space_t space, address_t address, char* buffer,
              size_t buffer_size);
  size_t write(space_t space, address_t address, const char* buffer,
               size_t buffer_size);

 private:
  File* getFile(space_t space);

 private:
  unordered_map<space_t, File*> files_;
  mutex fetch_file_lock_;
};