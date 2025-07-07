

#ifndef __CONTIGUOUS_ALLOCATOR_H__
#define __CONTIGUOUS_ALLOCATOR_H__


#include <vector>
#include <unordered_set>
#include <cstdint>


using ui32 = uint32_t;

template<typename T>
class ContiguousAllocator {
public:

    std::vector<T> data;
    std::unordered_set<ui32> free_list;

public:
    ContiguousAllocator() : data(), free_list() {};
    ~ContiguousAllocator() {};

    inline T &operator[](ui32 index) {
        return data[index];
    }
    inline const T &operator[](ui32 index) const {
        return data[index];
    }

    void reserve(ui32 sz) {
        data.reserve(sz);
    }

    ui32 alloc() {
        ui32 idx;
        if (!free_list.empty()) {
            auto it = free_list.begin();
            idx = *it;// free_list.back();
            free_list.erase(it);
        } else {
            idx = (ui32)data.size();
            data.resize(idx + 1);
        }
        return idx;
    }

    void dealloc(ui32 idx) {
        if (free_list.find(idx) != free_list.end()) {
            int wahotiahw = 0;
        }
        free_list.insert(idx);
    }

    void clear() {
        data.clear();
        free_list.clear();
    }
};





#endif //__CONTIGUOUS_ALLOCATOR_H__