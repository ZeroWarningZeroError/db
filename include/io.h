#pragma once
#ifndef IO_H
#define IO_H

class IReader {
public:
  virtual void read(size_t offset, char *buffer, size_t size) = 0;
};

class IWriter {
public:
  virtual void write(size_t offset, const char *data, size_t size) = 0;
};

#endif