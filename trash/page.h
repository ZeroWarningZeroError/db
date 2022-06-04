// #ifndef PAGE_H
// #define PAGE_H

// #include <cstddef>
// #include <cstdint>
// #include <memory>
// #include <iostream>
// #include <functional>
// #include <optional>

// #include "base.h"

// using namespace std;

// using StringViewCompareType = std::function<int(string_view, string_view)>;

// constexpr int DEFAULT_PAGE_SIZE = 16 * 1024;

// // 记录头
// struct RecordHeader {
//     uint16_t next;
//     uint16_t owned;
//     uint16_t delete_mask;
//     uint16_t len;
//     uint16_t key_len;
//     uint16_t val_len;
// };

// // 记录
// struct Record {
//     RecordHeader header;
//     string_view key;
//     string_view val;
// };

// // 页
// class Page {
// public:
//     uint32_t parent;
//     uint32_t prev;
//     uint32_t next;
//     uint16_t heap_top;
//     uint16_t free;
//     uint16_t use;
//     uint16_t directory;
//     uint16_t slots;
//     uint16_t page_type;
//     uint16_t node_size;
//     uint32_t size;

// private:

// public:
//     char* base_address() const noexcept;
//     bool has_next() const;
//     Record* next_record();
//     bool insert(string_view key, string_view val, const
//     StringViewCompareType& compare); bool erase(string_view key, const
//     StringViewCompareType& compare); optional<string_view> search(string_view
//     key, const StringViewCompareType& compare); int set_record(uint16_t
//     offset, Record* record) noexcept;

// public:
//     uint16_t alloc(uint16_t size) noexcept;
//     uint16_t slot(int pos) const noexcept;
//     uint16_t slot_offset(int pos) const noexcept;
//     uint16_t slot(int slot_no, int record_no);
//     uint16_t insert_slot(int slot_no, uint16_t address) noexcept;
//     uint16_t erase_slot(int slot_no) noexcept;
//     uint16_t locate_slot(string_view data, const StringViewCompareType&
//     compare) noexcept; void SplitTo(char* other_page_address); void
//     set_slot(int slot_no, uint16_t address) noexcept; void scan_use()
//     noexcept; void scan_free() noexcept; void scan_slots() noexcept;

// private:
//     void adjust() noexcept;
//     template<typename T>
//     T* get_arribute(uint16_t offset);
//     void move(uint16_t src_offset, uint16_t len, uint16_t dst_offset)
//     noexcept;

// public:
//     friend ostream& operator << (ostream& os, const Page& page);
//     friend ostream& operator << (ostream& os, const Page* page);
// };

// template<typename T>
// T* Page::get_arribute(uint16_t offset) {
//     if (offset == 0) {
//         return nullptr;
//     }
//     return reinterpret_cast<T*>(this->base_address() + offset);
// }

// // 初始化构造
// template<>
// struct Constructor<Page> {
//     template<typename... Args>
//     static Page* construct(void* address, Args... args) {
//         Page* page = (Page*)address;
//         *page = {args...};

//         // 设置基本属性
//         page->slots = 2;
//         page->size = DEFAULT_PAGE_SIZE;
//         page->use = sizeof(Page);

//         // 设置虚拟记录
//         Record min_record = {{sizeof(Page) + sizeof(RecordHeader) + 3, 1, 0,
//         3, 3, 0}, {"min"}, {""}}; Record max_record = {{0, 1, 0, 3, 3, 0},
//         {"max"}, {""}};

//         uint16_t offset = page->set_record(sizeof(Page), &min_record);
//         uint16_t address1 = sizeof(Page);
//         memcpy(page->base_address() + page->slot_offset(0), &address1,
//         sizeof(uint16_t));

//         uint16_t address2 = sizeof(Page) + offset;
//         offset += page->set_record(sizeof(Page) + offset, &max_record);
//         memcpy(page->base_address() + page->slot_offset(1), &address2,
//         sizeof(uint16_t));

//         page->heap_top = sizeof(Page) + offset;

//         return page;
//     }
// };

// ostream& operator << (ostream& os, const RecordHeader& page);
// ostream& operator << (ostream& os, const RecordHeader* page);
// ostream& operator << (ostream& os, const Record& page);
// ostream& operator << (ostream& os, const Record* page);

// #endif