#ifndef IWTBAM_BASE_H
#define IWTBAM_BASE_H

#include <functional>
#include <string>

using std::string;
using std::string_view;

template <typename T> struct Constructor {
  template <typename... Args> static T *construct(void *address, Args... args) {
    T *obj = reinterpret_cast<T *>(address);
    *obj = {args...};
    return obj;
  }
};

#endif