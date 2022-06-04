#pragma once

#include <functional>
#include <string_view>

using std::function;
using std::string_view;

using StringViewCompareType = std::function<int(string_view, string_view)>;
using Compare = std::function<int(string_view, string_view)>;

using address_t = uint64_t;

#define PAGE_SIZE (16 * 1024)