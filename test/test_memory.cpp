#include "test_common.hpp"

class Memory : public ::testing::Test {
protected:
    DebugAllocator* allocator;

    void SetUp() override {
        allocator = new DebugAllocator(DebugAllocator::GetGlobalInstance());
    }

    void TearDown() override {
        delete allocator;
    }
};

TEST_F(Memory, GetBuffer)
{
    upng_t *png = upng_new_from_file("./resources/checker_24bit.png");
    ASSERT_NE(nullptr, png);
    ASSERT_EQ(UPNG_EOK, upng_decode_default(png));

    const auto buffer = upng_get_frame_buffer(png);
    ASSERT_NE(nullptr, png);
    
    upng_free(png);

    ASSERT_EQ(0, allocator->allocationCount());
}

TEST_F(Memory, MoveBuffer)
{
    upng_t *png = upng_new_from_file("./resources/checker_24bit.png");
    ASSERT_NE(nullptr, png);
    ASSERT_EQ(UPNG_EOK, upng_decode_default(png));

    auto buffer = upng_move_frame_buffer(png);
    ASSERT_NE(nullptr, png);
    
    upng_free(png);
    ASSERT_EQ(1, allocator->allocationCount());
    ASSERT_TRUE(allocator->hasAllocated(buffer));

    allocator->deallocate(buffer);
}
