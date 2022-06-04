// #include "page.h"
// #include <cassert>
// #include <vector>
// #include <fmt/format.h>

// using std::cout;
// using std::endl;

// bool Page::has_next() const {
//     return this->use;
// }

// Record* Page::next_record() {
//     return nullptr;
// }

// /**
//  * @brief 插入一条记录
//  *
//  * @param key       插入数据的键值
//  * @param val       插入数据的值
//  * @param compare   比较函数
//  * @return true     插入成功
//  * @return false    插入失败
//  */
// bool Page::insert(string_view key, string_view val, const
// StringViewCompareType& compare) {
//     int hit_slot = this->locate_slot(key, compare);
//     uint16_t pre_address = this->slot(hit_slot - 1);
//     auto prev = this->get_arribute<RecordHeader>(pre_address);
//     RecordHeader* cur = nullptr;
//     // 查询插入位置
//     while (prev) {
//         cur = this->get_arribute<RecordHeader>(prev->next);
//         string_view data = {this->base_address() + prev->next +
//         sizeof(RecordHeader), static_cast<size_t>(cur->key_len)};
//         // 结束条件
//         // 1.查询槽后最后一条记录
//         // 2.找寻到第一条比插入键值大的记录
//         if (cur->owned || compare(data, key) <= 0) {
//             break;
//         }
//         prev = cur;
//     }
//     // 构建插入数据
//     uint16_t key_len = key.size();
//     uint16_t val_len = val.size();
//     uint16_t len = key_len + val_len;
//     Record need_insert_data = {{prev->next, 0, 0, len, key_len, val_len},
//     key, val};

//     // 开辟可用空间
//     uint16_t address = this->alloc(sizeof(RecordHeader) + len);
//     if (address == 0) {
//         return false;
//     }

//     // 插入数据
//     prev->next = address;
//     set_record(address, &need_insert_data);
//     auto own_header = this->get_arribute<RecordHeader>(this->slot(hit_slot));
//     own_header->owned += 1;

//     // 拆分目录页
//     if (own_header->owned >= 8) {
//         int half = own_header->owned / 2;
//         own_header->owned = own_header->owned - half;
//         uint16_t new_dir_address =
//         *get_arribute<uint16_t>(this->slot(hit_slot, half - 2)); auto
//         new_own_header = get_arribute<RecordHeader>(new_dir_address);
//         new_own_header->owned = half;
//         insert_slot(hit_slot, new_dir_address);
//     }

//     return true;
// }

// bool Page::erase(string_view key, const StringViewCompareType& compare) {

//     int hit_slot = this->locate_slot(key, compare);
//     uint16_t pre_address = this->slot(hit_slot - 1);
//     auto prev = this->get_arribute<RecordHeader>(pre_address);
//     RecordHeader* cur = nullptr;
//     string_view data;
//     while (prev) {
//         cur = get_arribute<RecordHeader>(prev->next);
//         data = {this->base_address() + prev->next + sizeof(RecordHeader),
//         static_cast<size_t>(cur->key_len)}; if (compare(key, data) == 0) {
//             break;
//         }
//         if (cur->owned) {
//             break;
//         }
//         prev = cur;
//     }

//     // 未命中
//     if (compare(key, data) != 0) {
//         cout << "key=" << key << " 未命中!" << endl;
//         return false;
//     }

//     uint16_t next_use_record = cur->next;
//     uint16_t need_delete_record = prev->next;
//     // 删除数据移出数据链表
//     prev->next = next_use_record;
//     auto own_header = this->get_arribute<RecordHeader>(this->slot(hit_slot));

//     // 调整目录
//     if (cur->owned && prev->owned) {
//         erase_slot(hit_slot);
//     } else {
//         if (cur->owned) {
//             own_header = prev;
//             own_header->owned = cur->owned;
//             set_slot(hit_slot, reinterpret_cast<char*>(prev) -
//             this->base_address());
//         }
//         own_header->owned--;
//     }

//     // 删除数据移入空闲链表中
//     auto free_header = this->get_arribute<RecordHeader>(offsetof(Page,
//     free)); cur->owned = 0; cur->delete_mask = 1; cur->next =
//     free_header->next; free_header->next = need_delete_record; return true;
// }

// optional<string_view> Page::search(string_view key, const
// StringViewCompareType& compare) {
//     uint16_t hit_slot = this->locate_slot(key, compare);
//     uint16_t pre_address = this->slot(hit_slot - 1);
//     auto prev = this->get_arribute<RecordHeader>(pre_address);
//     RecordHeader* cur = nullptr;
//     string_view data;
//     while (prev) {
//         cur = get_arribute<RecordHeader>(prev->next);
//         data = {this->base_address() + prev->next + sizeof(RecordHeader),
//         static_cast<size_t>(cur->key_len)}; if (compare(key, data) == 0) {
//             return {{this->base_address() + prev->next + sizeof(RecordHeader)
//             + cur->key_len, static_cast<size_t>(cur->val_len)}};
//         }
//         if (cur->owned) {
//             break;
//         }
//         prev = cur;
//     }
//     return std::nullopt;
// }

// /**
//  * @brief 在目录页中, 定位数据对应槽
//  *
//  * @param key       数据
//  * @param compare   对比函数
//  * @return uint16_t 返回数据对应槽所有指向的地址
//  */

// uint16_t Page::locate_slot(string_view key, const StringViewCompareType&
// compare) noexcept {
//     int lo = 0, hi = this->slots;
//     while (lo < hi) {
//         int mid = (lo + hi) >> 1;
//         uint16_t slotAddress = this->slot(mid);
//         auto header = this->get_arribute<RecordHeader>(this->slot(mid));

//         string_view data = {this->base_address() + slotAddress +
//         sizeof(RecordHeader), static_cast<size_t>(header->key_len)};

//         // mid = 0, 虚拟记录-最小记录
//         // mid = this->slots - 1, 虚拟记录-最大记录
//         // 0 < mid < this->slots - 1, 普通用户记录

//         int cmp_res = compare(data, key);

//         if (mid == 0 || mid == this->slots - 1) {
//             cmp_res = (mid ? -1 : 1);
//         }

//         if (cmp_res > 0) {
//             lo = mid + 1;
//         } else {
//             hi = mid;
//         }
//     }
//     return lo;
// }

// uint16_t Page::alloc(uint16_t size) noexcept {

//     uint16_t free_address = 0;
//     // “堆空间”有可用内存, 则在堆空间中开辟新空间
//     //  size + 4 预留两个“目录”位置
//     if (this->slot_offset(0) - this->heap_top >= size + 4) {
//         free_address = this->heap_top;
//         this->heap_top += size;
//         return free_address;
//     }

//     uint16_t free = this->free;
//     uint16_t total_free_size = 0;
//     // 遍历空闲链表, 找到可以复用空闲空间
//     auto pre = get_arribute<RecordHeader>(offsetof(Page, free));

//     while (pre && pre->next) {
//         uint16_t cur_free_address = pre->next;
//         auto cur = get_arribute<RecordHeader>(cur_free_address);
//         if (cur->len >= size) {
//             cur->delete_mask = 0;
//             pre->next = cur->next;
//             return cur_free_address;
//         }
//         total_free_size += cur->len;
//         pre = cur;
//     }

//     // 若无可以复用的空间, 但是总的空闲空间超过申请的空间,
//     则对整个“堆空间”进行清理 if (total_free_size > size) {
//         adjust();
//         free_address = this->heap_top;
//         this->heap_top += size;
//     }

//     return free_address;
// }

// char* Page::base_address() const noexcept {
//     return const_cast<char*>(reinterpret_cast<const char*>(this));
// }

// uint16_t Page::slot(int pos) const noexcept {
//     assert(pos >= 0 && pos < this->slots);
//     return *reinterpret_cast<uint16_t*>(this->base_address() + this->size -
//     this->slots * sizeof(uint16_t) + pos * sizeof(uint16_t));
// }

// uint16_t Page::slot_offset(int pos) const noexcept {
//     assert(pos >= 0 && pos < this->slots);
//     return this->size - this->slots * sizeof(uint16_t) + pos *
//     sizeof(uint16_t);
// }

// uint16_t Page::slot(int slot_no, int record_no) {
//     auto prev = get_arribute<RecordHeader>(this->slot(slot_no - 1));
//     int no = 0;
//     while (prev) {
//         auto cur = get_arribute<RecordHeader>(prev->next);
//         if (no == record_no) {
//             return prev->next;
//         }
//         no++;
//         prev = cur;
//     }
//     return 0;
// }

// uint16_t Page::insert_slot(int slot_no, uint16_t address) noexcept {
//     cout << "insert_slot:" << slot_no << " " << address << endl;
//     uint16_t src = this->slot_offset(0);
//     uint16_t dst = src - sizeof(uint16_t);
//     memcpy(this->base_address() + dst, this->base_address() + src,
//     sizeof(uint16_t) * slot_no); memcpy(this->base_address() + dst + slot_no
//     * sizeof(uint16_t), &address, sizeof(uint16_t)); this->slots++; return 0;
// }

// uint16_t Page::erase_slot(int slot_no) noexcept {
//     // this->size += sizeof(uint16_t);
//     cout << "erase_slot" << slot_no << endl;
//     uint16_t base_slot = this->slot_offset(0);
//     uint16_t src = base_slot;
//     uint16_t dst = base_slot + sizeof(uint16_t) * slot_no;
//     memcpy(this->base_address() + dst, this->base_address() + src,
//     sizeof(uint16_t) * slot_no); this->slots--; return 0;
// }

// void Page::set_slot(int slot_no, uint16_t address) noexcept {
//     uint16_t slot_address = this->slot_offset(slot_no);
//     memcpy(this->base_address() + slot_address, &address, sizeof(uint16_t));
//     return ;
// }

// int Page::set_record(uint16_t offset, Record* record) noexcept {
//     memcpy(this->base_address() + offset, &record->header,
//     sizeof(record->header)); memcpy(this->base_address() + offset +
//     sizeof(record->header), record->key.data(),record->header.key_len);
//     memcpy(this->base_address() + offset + sizeof(record->header) +
//     record->header.key_len, record->val.data(), record->header.val_len);
//     return sizeof(RecordHeader) + record->header.len;
// }

// void Page::scan_use() noexcept {
//     auto use = get_arribute<RecordHeader>(this->use);
//     uint16_t offset = this->use;
//     cout << "use=[" << endl;
//     while (use) {
//         string_view key = {this->base_address() + sizeof(RecordHeader) +
//         offset, static_cast<size_t>(use->key_len)}; string_view val =
//         {this->base_address() + sizeof(RecordHeader) + offset + use->key_len,
//         static_cast<size_t>(use->val_len)}; cout << " " << use <<
//         fmt::format("[key={}, val={}]", key, val) << endl; offset =
//         use->next; use = get_arribute<RecordHeader>(use->next);
//     }
//     cout << "]" << endl;
// }

// void Page::adjust() noexcept {
//     // todo 优化
//     char* buffer = new char[16 * 1024];
//     uint16_t* slots = new uint16_t[this->slots - 2];
//     uint16_t use = this->use;
//     uint16_t offset = 0;
//     uint16_t slot = 0;

//     // 保留页面设置和虚拟记录
//     uint16_t new_heap_top = sizeof(Page) + sizeof(RecordHeader) * 2 + 2 * 3;
//     auto prev = get_arribute<RecordHeader>(use);
//     uint16_t next_record_address = prev->next;
//     uint16_t pre_record_address = prev->next;

//     int no = 0;
//     while (next_record_address >= new_heap_top) {
//         auto cur = get_arribute<RecordHeader>(next_record_address);
//         uint16_t occupy = sizeof(RecordHeader) + cur->len;
//         pre_record_address = next_record_address;
//         next_record_address = cur->next;
//         if (cur->next > new_heap_top) {
//             cur->next = new_heap_top + offset + occupy;
//         }
//         memcpy(buffer + offset, cur, occupy);
//         if (cur->owned) {
//             slots[slot++] = new_heap_top + offset;
//         }
//         offset += occupy;
//     }

//     this->use = sizeof(Page);
//     this->free = 0;
//     auto min_record =  get_arribute<RecordHeader>(this->use);
//     min_record->next = new_heap_top;
//     this->heap_top = new_heap_top;

//     memcpy(this->base_address() + this->heap_top, buffer, offset);
//     memcpy(this->base_address() + this->slot_offset(1), slots,
//     sizeof(uint16_t) * (this->slots - 2));

//     this->heap_top += offset;
//     delete[] buffer;
//     delete[] slots;
// }

// void Page::scan_free() noexcept {
//     auto free = get_arribute<RecordHeader>(this->free);
//     uint16_t offset = this->free;
//     cout << "free=[" << endl;
//     int no = 0;
//     while (free) {
//         string_view key = {this->base_address() + sizeof(RecordHeader) +
//         offset, static_cast<size_t>(free->key_len)}; string_view val =
//         {this->base_address() + sizeof(RecordHeader) + offset +
//         free->key_len, static_cast<size_t>(free->val_len)}; cout << " " <<
//         free << fmt::format("[key={}, val={}]", key, val) << endl; offset =
//         free->next; free = get_arribute<RecordHeader>(free->next); if (no++ >
//         3) {
//             break;
//         }
//     }
//     cout << "]" << endl;
// }

// void Page::scan_slots() noexcept {
//     cout << "slots=[" << endl;
//     for (int i = 0; i < this->slots; i++) {
//         auto header = get_arribute<RecordHeader>(this->slot(i));
//         string_view key = {this->base_address() + this->slot(i) +
//         sizeof(RecordHeader), static_cast<size_t>(header->key_len)}; cout <<
//         " [i=" << i << ",offset="<< this->slot_offset(i) << ",header=" <<
//         this->slot(i)<< "," << header << ", " << key << "]" << endl;
//     }
//     cout << "]" << endl;
// }

// void Page::SplitTo(char* other_page_address) {

// }

// ostream& operator << (ostream& os, const Page& page) {
//     string slots;
//     for (int i = 0; i < page.slots; i++) {
//         slots += to_string(page.slot(i));
//         if (i != page.slots - 1) {
//             slots += ",";
//         }
//     }
//     return os << fmt::format("[type=Page, fields=[prev={}, next={},
//     heap_top={}[{}], free={}, use={}, slots=[{}]]]", page.prev, page.next,
//     page.heap_top, sizeof(Page), page.free, page.use, slots);
// }

// ostream& operator << (ostream& os, const Page* page) {
//     return os << (*page);
// }

// ostream& operator << (ostream& os, const RecordHeader& header) {
//     return os << fmt::format("[type=RecordHeader, fields=[next={}, own={},
//     delete_mask={}, len={}, key_len={}, val_len={}]]", header.next,
//     header.owned, header.delete_mask, header.len, header.key_len,
//     header.val_len);
// }

// ostream& operator << (ostream& os, const RecordHeader* header) {
//     return os << (*header);
// }

// ostream& operator << (ostream& os, const Record& record) {
//     return os;
// }

// ostream& operator << (ostream& os, const Record* record) {
//     return os;
// }