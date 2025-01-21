#include "test_common.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <filesystem>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif//_WIN32

class MultipleFrames : public ::testing::Test {};

static int upng_inflate(char* out, int outsize, const char* in, int insize)
{
    return stbi_zlib_decode_buffer(out, outsize, in, insize) == outsize ? 0 : -1;
}

class InitDeflate {
public:
    InitDeflate()
    {
        upng_set_inflate(upng_inflate);
        char path[] = __FILE__;
#ifdef _WIN32
        *strrchr(path, '\\')=0;
        SetCurrentDirectoryA(path);
#else
        * strrchr(path, '/') = 0;
        chdir(path);
#endif
    };
};

InitDeflate g_initDeflate;// NOLINT

TEST_F(MultipleFrames, WithoutDefaultImage)
{
    // only tests meta behaviour, content is tested elsewhere
    upng_t* upng = upng_new_from_file("./resources/excors/025.png");
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
    upng_t* upng = upng_new_from_file("./resources/excors/025.png");
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
