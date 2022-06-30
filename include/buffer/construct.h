#ifndef CONSTRUCT_H
#define CONSTRUCT_H

template <typename T>
struct FrameConstructor {
  template <typename... Args>
  static T *Construct(Frame *frame, Args... args) {
    T *obj = reinterpret_cast<T *>(frame->buffer);
    *obj = {std::forward<Args>(args)...};
    return obj;
  }
};

template <typename T>
struct FrameDeconstructor {
  static void Deconstruct(T *data) { return; }
};

#endif