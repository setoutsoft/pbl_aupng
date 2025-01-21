#pragma once
#include <memory>
#include <vector>
#include <cstddef>

class DebugAllocator
{
public:
    static DebugAllocator* GetGlobalInstance();

    DebugAllocator(DebugAllocator* parent = nullptr);
    ~DebugAllocator() noexcept(false);

    void* allocate(size_t size, const char* file = nullptr, int line = -1);
    void deallocate(void* ptr);

    size_t allocationCount() const;
    size_t allocationSize() const;
    bool hasAllocated(const void* ptr) const;
private:
    struct Block
    {
        // this is an owning pointer depending on whether
        // this is a child allocator (in which case it is not owning)
        unsigned char* start;
        size_t size;
        const char* file;
        int line;

        Block(unsigned char* start = nullptr, size_t size = 0, const char* file = nullptr, int line = -1);
        bool operator == (const Block& other) const;
        bool isInside(const void* ptr) const;
    };

    void registerChild(DebugAllocator* child);
    void unregisterChild(DebugAllocator* child);
    void onParentAllocation(Block block);
    void onParentDeallocation(Block block);

    std::vector<Block> known_blocks;
    std::vector<DebugAllocator*> children;
    DebugAllocator* parent;
};
