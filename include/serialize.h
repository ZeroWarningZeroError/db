#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <string>

using std::string;

template <typename T>
struct Serializer {
  static string serialize(T obj) {
    string data(sizeof(T), 0);
    memcpy(data.data(), &obj, sizeof(T));
    return data;
  }

  static T deserialize(const string &data) {
    return *reinterpret_cast<const T *>(data.data());
  }
};

#endif