#include "test_common.hpp"

class MultipleFrames : public ::testing::Test {};

TEST_F(MultipleFrames, WithoutDefaultImage)
{
    // only tests meta behaviour, content is tested elsewhere
    upng_t* upng = upng_new_from_file("test/resources/excors/025.png");
    ASSERT_NE(nullptr, upng);
    ASSERT_EQ(UPNG_EOK, upng_header(upng));
    ASSERT_EQ(0, upng_get_plays(upng));
    ASSERT_EQ(4, upng_get_frame_count(upng));
    ASSERT_LT(4, upng_get_frame_index(upng));

    ASSERT_EQ(UPNG_EOK, upng_decode_default(upng));
    ASSERT_LT(4, upng_get_frame_index(upng));
    upng_rect defaultRect, curRect;
    upng_get_rect(upng, &defaultRect);
    upng_get_frame_rect(upng, &curRect);
    ASSERT_EQ(defaultRect, curRect);

    ASSERT_EQ(UPNG_EOK, upng_decode_next_frame(upng));
    ASSERT_EQ(0, upng_get_frame_index(upng));
    upng_get_frame_rect(upng, &curRect);
    ASSERT_EQ(defaultRect, curRect);

    ASSERT_EQ(UPNG_EOK, upng_decode_next_frame(upng));
    ASSERT_EQ(1, upng_get_frame_index(upng));
    upng_get_frame_rect(upng, &curRect);
    ASSERT_EQ(defaultRect, curRect);

    ASSERT_EQ(UPNG_EOK, upng_decode_next_frame(upng));
    ASSERT_EQ(2, upng_get_frame_index(upng));
    upng_get_frame_rect(upng, &curRect);
    ASSERT_EQ(defaultRect, curRect);

    ASSERT_EQ(UPNG_EOK, upng_decode_next_frame(upng));
    ASSERT_EQ(3, upng_get_frame_index(upng));
    upng_get_frame_rect(upng, &curRect);
    ASSERT_EQ(defaultRect, curRect);

    upng_free(upng);
}

TEST_F(MultipleFrames, FramesWrapAround)
{
    upng_t* upng = upng_new_from_file("test/resources/excors/025.png");
    ASSERT_NE(nullptr, upng);
    ASSERT_EQ(UPNG_EOK, upng_header(upng));
    ASSERT_EQ(4, upng_get_frame_count(upng));

    ASSERT_EQ(UPNG_EOK, upng_decode_next_frame(upng));
    ASSERT_EQ(UPNG_EOK, upng_decode_next_frame(upng));
    ASSERT_EQ(UPNG_EOK, upng_decode_next_frame(upng));
    ASSERT_EQ(UPNG_EOK, upng_decode_next_frame(upng));
    ASSERT_EQ(UPNG_EOK, upng_decode_next_frame(upng));
    ASSERT_EQ(0, upng_get_frame_index(upng));

    upng_free(upng);
}
