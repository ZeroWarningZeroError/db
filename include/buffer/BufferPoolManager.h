#pragma once
#ifndef BUFFER_POOL_MANAGER
#define BUFFER_POOL_MANAGER

class IBufferPool {
 public:
  virtual void* FetchPage(table_space_id_t table_space_id,
                          address_t page_address) = 0;
};

#endif