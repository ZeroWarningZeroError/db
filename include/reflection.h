#ifndef IWTBAM_REFLECTION_H
#define IWTBAM_REFLECTION_H

struct AnyType {
  template <typename T> operator T();
};

// template <typename T, typename...Args>
// consteval size_t CountMember(Args&&... args) {
//     if constexpr (! requires { T{ args... }; }) { // (1)
//         return sizeof...(args) - 1;
//     } else {
//         return CountMember<T>(args..., AnyType{}); // (2)
//     }
// }

#endif