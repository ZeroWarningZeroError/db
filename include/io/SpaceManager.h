#pragma once

class ISpaceManager {
 public:
  virtual size_t read(space_t space, address_t address, char *buffer,
                      size_t buffer_size) = 0;
  virtual size_t write(space_t space, address_t address, const char *buffer,
                       size_t buffer_size) = 0;
};