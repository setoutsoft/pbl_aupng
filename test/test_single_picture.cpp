#include <gtest/gtest.h>
extern "C" {
#include "../src/upng.h"
}

class SinglePicture : public ::testing::Test {};

TEST_F(SinglePicture, Load8Bit)
{
    static const uint8_t pixels[] = {
        0xff, 0xff, 0xff,
        0x00, 0x00, 0x00,
        0xff, 0x00, 0x00,
        0x00, 0xff, 0x00
    };
    upng_t *png = upng_new_from_file("test/resources/checker_8bit.png");
    ASSERT_NE(nullptr, png);
    ASSERT_EQ(UPNG_EOK, upng_get_error(png));

    ASSERT_EQ(UPNG_EOK, upng_header(png));
    ASSERT_EQ(2, upng_get_width(png));
    ASSERT_EQ(2, upng_get_height(png));
    ASSERT_EQ(24, upng_get_bpp(png));
    ASSERT_EQ(8, upng_get_bitdepth(png));
    ASSERT_EQ(3, upng_get_components(png));
    ASSERT_EQ(UPNG_RGB8, upng_get_format(png));

    ASSERT_EQ(UPNG_EOK, upng_decode(png));
    ASSERT_EQ(0, memcmp(upng_get_buffer(png), pixels, sizeof(pixels)));
}

TEST_F(SinglePicture, Load2Bit)
{
    static const uint8_t palette[] = {
        0xff, 0xff, 0xff,
        0x00, 0x00, 0x00,
        0xff, 0x00, 0x00,
        0x00, 0xff, 0x00
    };
    static const uint8_t pixels[] = {
        0b00010000,
        0b10110000
    };
    upng_t *png = upng_new_from_file("test/resources/checker_2bit.png");
    ASSERT_NE(nullptr, png);
    ASSERT_EQ(UPNG_EOK, upng_get_error(png));

    ASSERT_EQ(UPNG_EOK, upng_header(png));
    ASSERT_EQ(2, upng_get_width(png));
    ASSERT_EQ(2, upng_get_height(png));
    ASSERT_EQ(2, upng_get_bpp(png));
    ASSERT_EQ(2, upng_get_bitdepth(png));
    ASSERT_EQ(1, upng_get_components(png));
    ASSERT_EQ(UPNG_INDEXED2, upng_get_format(png));

    rgb* palettePtr;
    uint8_t* alphaPtr;
    ASSERT_EQ(UPNG_EOK, upng_decode(png));
    ASSERT_EQ(4, upng_get_palette(png, &palettePtr));
    ASSERT_EQ(0, memcmp(palettePtr, palette, sizeof(palette)));
    ASSERT_EQ(0, upng_get_alpha(png, &alphaPtr));
    ASSERT_EQ(0, memcmp(upng_get_buffer(png), pixels, sizeof(pixels)));
}

TEST_F(SinglePicture, Load1Bit)
{
    static const uint8_t pixels[] = {
        0b10000000,
        0b01000000
    };
    upng_t *png = upng_new_from_file("test/resources/checker_1bit.png");
    ASSERT_NE(nullptr, png);
    ASSERT_EQ(UPNG_EOK, upng_get_error(png));

    ASSERT_EQ(UPNG_EOK, upng_header(png));
    ASSERT_EQ(2, upng_get_width(png));
    ASSERT_EQ(2, upng_get_height(png));
    ASSERT_EQ(1, upng_get_bpp(png));
    ASSERT_EQ(1, upng_get_bitdepth(png));
    ASSERT_EQ(1, upng_get_components(png));
    ASSERT_EQ(UPNG_LUMINANCE1, upng_get_format(png));

    ASSERT_EQ(UPNG_EOK, upng_decode(png));
    ASSERT_EQ(0, memcmp(upng_get_buffer(png), pixels, sizeof(pixels)));
}
