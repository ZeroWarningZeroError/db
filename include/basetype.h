#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

using std::function;
using std::same_as;
using std::string;
using std::string_view;
using StringViewCompareType = std::function<int(string_view, string_view)>;
using Compare = std::function<int(string_view, string_view)>;

/**
 * @brief 比较器型别约束
 *
 * @tparam Type 检查类型
 */
template <typename Type>
concept Comparator = requires(Type& comparator, string_view& lhs,
                              string_view& rhs) {
  { comparator(lhs, rhs) }
  ->same_as<int>;
};

using address_t = int64_t;
using page_id_t = int64_t;
using table_id_t = int64_t;
using frame_id_t = int64_t;
using space_t = string;

#ifndef PAGE_SIZE
#define PAGE_SIZE (16 * 20)
#endif