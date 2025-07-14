

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
    std::vector<ui32> free_list;

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
        free_list.reserve(sz);
    }

    ui32 alloc() {
        ui32 idx;
        if (!free_list.empty()) {
            idx = free_list.back();
            free_list.pop_back();
        } else {
            idx = (ui32)data.size();
            data.resize(idx + 1);
        }
        return idx;
    }

    void dealloc(ui32 idx) {
        free_list.push_back(idx);
    }

    void clear() {
        data.clear();
        free_list.clear();
    }
};





#endif //__CONTIGUOUS_ALLOCATOR_H__