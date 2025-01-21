#include "DebugAllocator.hpp"
#include <exception>
#include <algorithm>
#include <stdexcept>

namespace
{
    static DebugAllocator global_debug_allocator;
}
extern "C"
{
    void* upng_mem_alloc(unsigned size, const char* file, int line)
    {
        return global_debug_allocator.allocate(static_cast<size_t>(size), file, line);
    }

    void upng_mem_free(void* ptr)
    {
        return global_debug_allocator.deallocate(ptr);
    }
}
DebugAllocator* DebugAllocator::GetGlobalInstance()
{
    return &global_debug_allocator;
}

DebugAllocator::Block::Block(unsigned char* _start, size_t _size, const char* _file, int _line) :
    start(_start), size(_size), file(_file), line(_line) {}

bool DebugAllocator::Block::operator== (const Block& other) const
{
    return this->start == other.start && this->size == other.size;
}

bool DebugAllocator::Block::isInside(const void* ptr) const
{
    return ptr >= start && ptr < start + size;
}

DebugAllocator::DebugAllocator(DebugAllocator* _parent) : parent(_parent)
{
    if (parent != nullptr)
        parent->registerChild(this);
}

DebugAllocator::~DebugAllocator() noexcept(false)
{
    if (!children.empty())
        throw std::runtime_error("Parent debug allocator is freed before its children are");

    if (parent != nullptr)
        parent->unregisterChild(this);
    else if (!known_blocks.empty())
    {
        // don't throw, this only messes up googletest
        // instead have tests that check for memory leaks
        for (auto& block : known_blocks)
            delete[] block.start;
    }
}

void* DebugAllocator::allocate(size_t size, const char* file, int line)
{
    Block block;
    if (parent == nullptr)
        known_blocks.emplace_back(new unsigned char[size], size, file, line);
    else
    {
        parent->allocate(size, file, line);
    }
    block = known_blocks.back();

    for (auto& child : children)
        child->onParentAllocation(block);

    return block.start;
}

void DebugAllocator::deallocate(void* ptr)
{
    if (parent != nullptr)
    {
        parent->deallocate(ptr);
        return;
    }

    auto itBlock = std::find_if(known_blocks.begin(), known_blocks.end(), 
        [=](auto& b) { return b.start == ptr; });
    if (itBlock == known_blocks.end())
        throw std::runtime_error("Tried to deallocate invalid pointer");

    for (auto& child : children)
        child->onParentDeallocation(*itBlock);

    delete[] itBlock->start;
    known_blocks.erase(itBlock);
}

void DebugAllocator::registerChild(DebugAllocator* child)
{
    children.push_back(child);
}

void DebugAllocator::unregisterChild(DebugAllocator* child)
{
    auto itChild = std::find(children.begin(), children.end(), child);
    if (itChild != children.end())
        children.erase(itChild);
}

void DebugAllocator::onParentAllocation(Block block)
{
    known_blocks.push_back(block);
}

void DebugAllocator::onParentDeallocation(Block block)
{
    auto itBlock = std::find(known_blocks.begin(), known_blocks.end(), block);
    if (itBlock != known_blocks.end())
        known_blocks.erase(itBlock);
}

size_t DebugAllocator::allocationCount() const
{
    return known_blocks.size();
}

size_t DebugAllocator::allocationSize() const
{
    size_t size = 0;
    for (auto& block : known_blocks)
        size += block.size;
    return size;
}

bool DebugAllocator::hasAllocated(const void* ptr) const
{
    auto itBlock = std::find_if(known_blocks.begin(), known_blocks.end(),
        [=](auto& b) { return b.isInside(ptr); });
    return itBlock != known_blocks.end();
}
