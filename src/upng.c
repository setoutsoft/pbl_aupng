/*
uPNG -- derived from LodePNG version 20100808

Copyright (c) 2005-2010 Lode Vandevenne
Copyright (c) 2010 Sean Middleditch

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

                1. The origin of this software must not be misrepresented; you must not
                claim that you wrote the original software. If you use this software
                in a product, an acknowledgment in the product documentation would be
                appreciated but is not required.

                2. Altered source versions must be plainly marked as such, and must not be
                misrepresented as being the original software.

                3. This notice may not be removed or altered from any source
                distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "upng.h"

extern upng_error uz_inflate(unsigned char *out, unsigned long outsize, const unsigned char *in, unsigned long insize);

#define MAKE_BYTE(b) ((b)&0xFF)
#define MAKE_DWORD(a, b, c, d) ((MAKE_BYTE(a) << 24) | (MAKE_BYTE(b) << 16) | (MAKE_BYTE(c) << 8) | MAKE_BYTE(d))
#define MAKE_DWORD_PTR(p) MAKE_DWORD((p)[0], (p)[1], (p)[2], (p)[3])

#define CHUNK_IHDR MAKE_DWORD('I', 'H', 'D', 'R')
#define CHUNK_IDAT MAKE_DWORD('I', 'D', 'A', 'T')
#define CHUNK_TEXT MAKE_DWORD('t', 'E', 'X', 't')
#define CHUNK_tRNS MAKE_DWORD('t', 'R', 'N', 'S')
#define CHUNK_PLTE MAKE_DWORD('P', 'L', 'T', 'E')
#define CHUNK_OFFS MAKE_DWORD('o', 'F', 'F', 's')
#define CHUNK_IEND MAKE_DWORD('I', 'E', 'N', 'D')

#define SET_ERROR(upng, code)          \
    do                                 \
    {                                  \
        (upng)->error = (code);        \
        (upng)->error_line = __LINE__; \
    } while (0)

#define upng_chunk_length(chunk) MAKE_DWORD_PTR(chunk)
#define upng_chunk_type(chunk) MAKE_DWORD_PTR((chunk) + 4)
#define upng_chunk_data(chunk) ((chunk) + 8)
#define upng_chunk_critical(chunk) (((chunk)[4] & 32) == 0)

typedef enum upng_state
{
    UPNG_ERROR = -1,
    UPNG_DECODED = 0,
    UPNG_HEADER = 1,
    UPNG_NEW = 2
} upng_state;

typedef enum upng_color
{
    UPNG_LUM = 0,
    UPNG_RGB = 2,
    UPNG_PLT = 3,
    UPNG_LUMA = 4,
    UPNG_RGBA = 6
} upng_color;

typedef struct upng_text
{
    char *keyword;
    char *text;
} upng_text;

struct upng_t
{
    unsigned width;
    unsigned height;

    int x_offset;
    int y_offset;

    rgb *palette;
    unsigned char palette_entries;

    uint8_t *alpha;
    unsigned char alpha_entries;

    upng_color color_type;
    unsigned color_depth;
    upng_format format;

    unsigned char *buffer;
    unsigned long size;

    upng_text text[10];
    unsigned int text_count;

    upng_error error;
    unsigned error_line;

    upng_state state;
    upng_source source;
};

/*Paeth predicter, used by PNG filter type 4*/
static int paeth_predictor(int a, int b, int c)
{
    int p = a + b - c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;

    if (pa <= pb && pa <= pc)
        return a;
    else if (pb <= pc)
        return b;
    else
        return c;
}

static void unfilter_scanline(upng_t *upng, unsigned char *recon, const unsigned char *scanline, const unsigned char *precon, unsigned long bytewidth, unsigned char filterType, unsigned long length)
{
    /*
        For PNG filter method 0
        unfilter a PNG image scanline by scanline. when the pixels are smaller than 1 byte, the filter works byte per byte (bytewidth = 1)
        precon is the previous unfiltered scanline, recon the result, scanline the current one
        the incoming scanlines do NOT include the filtertype byte, that one is given in the parameter filterType instead
        recon and scanline MAY be the same memory address! precon must be disjoint.
        */

    unsigned long i;
    switch (filterType)
    {
    case 0:
        for (i = 0; i < length; i++)
            recon[i] = scanline[i];
        break;
    case 1:
        for (i = 0; i < bytewidth; i++)
            recon[i] = scanline[i];
        for (i = bytewidth; i < length; i++)
            recon[i] = scanline[i] + recon[i - bytewidth];
        break;
    case 2:
        if (precon)
            for (i = 0; i < length; i++)
                recon[i] = scanline[i] + precon[i];
        else
            for (i = 0; i < length; i++)
                recon[i] = scanline[i];
        break;
    case 3:
        if (precon)
        {
            for (i = 0; i < bytewidth; i++)
                recon[i] = scanline[i] + precon[i] / 2;
            for (i = bytewidth; i < length; i++)
                recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
        }
        else
        {
            for (i = 0; i < bytewidth; i++)
                recon[i] = scanline[i];
            for (i = bytewidth; i < length; i++)
                recon[i] = scanline[i] + recon[i - bytewidth] / 2;
        }
        break;
    case 4:
        if (precon)
        {
            for (i = 0; i < bytewidth; i++)
                recon[i] = (unsigned char)(scanline[i] + paeth_predictor(0, precon[i], 0));
            for (i = bytewidth; i < length; i++)
                recon[i] = (unsigned char)(scanline[i] + paeth_predictor(recon[i - bytewidth], precon[i], precon[i - bytewidth]));
        }
        else
        {
            for (i = 0; i < bytewidth; i++)
                recon[i] = scanline[i];
            for (i = bytewidth; i < length; i++)
                recon[i] = (unsigned char)(scanline[i] + paeth_predictor(recon[i - bytewidth], 0, 0));
        }
        break;
    default:
        SET_ERROR(upng, UPNG_EMALFORMED);
        break;
    }
}

static void unfilter(upng_t *upng, unsigned char *out, const unsigned char *in, unsigned w, unsigned h, unsigned bpp)
{
    /*
        For PNG filter method 0
        this function unfilters a single image (e.g. without interlacing this is called once, with Adam7 it's called 7 times)
        out must have enough bytes allocated already, in must have the scanlines + 1 filtertype byte per scanline
        w and h are image dimensions or dimensions of reduced image, bpp is bpp per pixel
        in and out are allowed to be the same memory address!
        */

    unsigned y;
    unsigned char *prevline = 0;

    unsigned long bytewidth = (bpp + 7) / 8; /*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise */
    unsigned long linebytes = (w * bpp + 7) / 8;

    for (y = 0; y < h; y++)
    {
        unsigned long outindex = linebytes * y;
        unsigned long inindex = (1 + linebytes) * y; /*the extra filterbyte added to each row */
        unsigned char filterType = in[inindex];

        unfilter_scanline(upng, &out[outindex], &in[inindex + 1], prevline, bytewidth, filterType, linebytes);
        if (upng->error != UPNG_EOK)
        {
            return;
        }

        prevline = &out[outindex];
    }
}

static void remove_padding_bits(unsigned char *out, const unsigned char *in, unsigned long olinebits, unsigned long ilinebits, unsigned h)
{
    /*
        After filtering there are still padding bpp if scanlines have non multiple of 8 bit amounts. They need to be removed (except at last scanline of (Adam7-reduced) image) before working with pure image buffers for the Adam7 code, the color convert code and the output to the user.
        in and out are allowed to be the same buffer, in may also be higher but still overlapping; in must have >= ilinebits*h bpp, out must have >= olinebits*h bpp, olinebits must be <= ilinebits
        also used to move bpp after earlier such operations happened, e.g. in a sequence of reduced images from Adam7
        only useful if (ilinebits - olinebits) is a value in the range 1..7
        */
    unsigned y;
    unsigned long diff = ilinebits - olinebits;
    unsigned long obp = 0, ibp = 0; /*bit pointers */
    for (y = 0; y < h; y++)
    {
        unsigned long x;
        for (x = 0; x < olinebits; x++)
        {
            unsigned char bit = (unsigned char)((in[(ibp) >> 3] >> (7 - ((ibp)&0x7))) & 1);
            ibp++;

            if (bit == 0)
                out[(obp) >> 3] &= (unsigned char)(~(1 << (7 - ((obp)&0x7))));
            else
                out[(obp) >> 3] |= (1 << (7 - ((obp)&0x7)));
            ++obp;
        }
        ibp += diff;
    }
}

/*out must be buffer big enough to contain full image, and in must contain the full decompressed data from the IDAT chunks*/
static void post_process_scanlines(upng_t *upng, unsigned char *out, unsigned char *in, const upng_t *info_png)
{
    unsigned bpp = upng_get_bpp(info_png);
    unsigned w = info_png->width;
    unsigned h = info_png->height;

    if (bpp == 0)
    {
        SET_ERROR(upng, UPNG_EMALFORMED);
        return;
    }

    if (bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8)
    {
        unfilter(upng, in, in, w, h, bpp);
        if (upng->error != UPNG_EOK)
        {
            return;
        }
        // remove_padding_bits(out, in, w * bpp, ((w * bpp + 7) / 8) * 8, h);
        // fix for non-byte-aligned images
        unsigned aligned_width = ((w * bpp + 7) / 8) * 8;
        remove_padding_bits(in, in, aligned_width, aligned_width, h);
    }
    else
    {
        unfilter(upng, in, in, w, h, bpp); /*we can immediatly filter into the out buffer, no other steps needed */
    }
}

static upng_format determine_format(upng_t *upng)
{
    switch (upng->color_type)
    {
    case UPNG_PLT:
        switch (upng->color_depth)
        {
        case 1:
            return UPNG_INDEXED1;
        case 2:
            return UPNG_INDEXED2;
        case 4:
            return UPNG_INDEXED4;
        case 8:
            return UPNG_INDEXED8;
        default:
            return UPNG_BADFORMAT;
        }
    case UPNG_LUM:
        switch (upng->color_depth)
        {
        case 1:
            return UPNG_LUMINANCE1;
        case 2:
            return UPNG_LUMINANCE2;
        case 4:
            return UPNG_LUMINANCE4;
        case 8:
            return UPNG_LUMINANCE8;
        default:
            return UPNG_BADFORMAT;
        }
    case UPNG_RGB:
        switch (upng->color_depth)
        {
        case 8:
            return UPNG_RGB8;
        case 16:
            return UPNG_RGB16;
        default:
            return UPNG_BADFORMAT;
        }
    case UPNG_LUMA:
        switch (upng->color_depth)
        {
        case 1:
            return UPNG_LUMINANCE_ALPHA1;
        case 2:
            return UPNG_LUMINANCE_ALPHA2;
        case 4:
            return UPNG_LUMINANCE_ALPHA4;
        case 8:
            return UPNG_LUMINANCE_ALPHA8;
        default:
            return UPNG_BADFORMAT;
        }
    case UPNG_RGBA:
        switch (upng->color_depth)
        {
        case 8:
            return UPNG_RGBA8;
        case 16:
            return UPNG_RGBA16;
        default:
            return UPNG_BADFORMAT;
        }
    default:
        return UPNG_BADFORMAT;
    }
}

static void upng_free_source(upng_t *upng)
{
    if (upng->source.free != NULL)
        upng->source.free(upng->source.user);
    memset(&upng->source, 0, sizeof(upng->source));
}

/*read the information from the header and store it in the upng_Info. return value is error*/
upng_error upng_header(upng_t *upng)
{
    /* if we have an error state, bail now */
    if (upng->error != UPNG_EOK)
    {
        return upng->error;
    }

    /* if the state is not NEW (meaning we are ready to parse the header), stop now */
    if (upng->state != UPNG_NEW)
    {
        return upng->error;
    }

    /* minimum length of a valid PNG file is 29 bytes
        * FIXME: verify this against the specification, or
        * better against the actual code below */
    if (upng->source.size < 29)
    {
        SET_ERROR(upng, UPNG_ENOTPNG);
        return upng->error;
    }
    unsigned char header[29];
    if (upng->source.read(upng->source.user, 0, header, 29) != 29)
    {
        SET_ERROR(upng, UPNG_EREAD);
        return upng->error;
    }

    /* check that PNG header matches expected value */
    if (header[0] != 137 || header[1] != 80 || header[2] != 78 || header[3] != 71 || header[4] != 13 || header[5] != 10 || header[6] != 26 || header[7] != 10)
    {
        SET_ERROR(upng, UPNG_ENOTPNG);
        return upng->error;
    }

    /* check that the first chunk is the IHDR chunk */
    if (MAKE_DWORD_PTR(header + 12) != CHUNK_IHDR)
    {
        SET_ERROR(upng, UPNG_EMALFORMED);
        return upng->error;
    }

    /* read the values given in the header */
    upng->width = MAKE_DWORD_PTR(header + 16);
    upng->height = MAKE_DWORD_PTR(header + 20);
    upng->color_depth = header[24];
    upng->color_type = (upng_color)header[25];

    /* determine our color format */
    upng->format = determine_format(upng);
    if (upng->format == UPNG_BADFORMAT)
    {
        SET_ERROR(upng, UPNG_EUNFORMAT);
        return upng->error;
    }

    /* check that the compression method (byte 27) is 0 (only allowed value in spec) */
    if (header[26] != 0)
    {
        SET_ERROR(upng, UPNG_EMALFORMED);
        return upng->error;
    }

    /* check that the compression method (byte 27) is 0 (only allowed value in spec) */
    if (header[27] != 0)
    {
        SET_ERROR(upng, UPNG_EMALFORMED);
        return upng->error;
    }

    /* check that the compression method (byte 27) is 0 (spec allows 1, but uPNG does not support it) */
    if (header[28] != 0)
    {
        SET_ERROR(upng, UPNG_EUNINTERLACED);
        return upng->error;
    }

    upng->state = UPNG_HEADER;
    return upng->error;
}

/*read a PNG, the result will be in the same color type as the PNG (hence "generic")*/
upng_error upng_decode(upng_t *upng)
{
    unsigned char *compressed;
    unsigned char *inflated;
    unsigned long compressed_size = 0, compressed_index = 0;
    unsigned long inflated_size;
    unsigned long chunk_offset;
    unsigned char chunk_header[12];
    upng_error error;

    /* if we have an error state, bail now */
    if (upng->error != UPNG_EOK)
    {
        return upng->error;
    }

    /* parse the main header, if necessary */
    upng_header(upng);
    if (upng->error != UPNG_EOK)
    {
        return upng->error;
    }

    /* if the state is not HEADER (meaning we are ready to decode the image), stop now */
    if (upng->state != UPNG_HEADER)
    {
        return upng->error;
    }

    /* release old result, if any */
    if (upng->buffer != 0)
    {
        app_free(upng->buffer);
        upng->buffer = 0;
        upng->size = 0;
    }

    /* first byte of the first chunk after the header */
    chunk_offset = 33;

    /* scan through the chunks, finding the size of all IDAT chunks, and also
        * verify general well-formed-ness */
    while (chunk_offset < upng->source.size)
    {
        unsigned long chunk_data_offset = upng_chunk_data(chunk_offset);
        unsigned long length;

        /* make sure chunk header is not larger than the total compressed */
        if (chunk_offset + 12 > upng->source.size)
        {
            SET_ERROR(upng, UPNG_EMALFORMED);
            return upng->error;
        }

        /* read chunk header */
        if (upng->source.read(upng->source.user, chunk_offset, chunk_header, 12) != 12)
        {
            SET_ERROR(upng, UPNG_EREAD);
            return upng->error;
        }

        /* get length; sanity check it */
        length = upng_chunk_length(chunk_header);
        if (length > INT_MAX)
        {
            SET_ERROR(upng, UPNG_EMALFORMED);
            return upng->error;
        }

        /* make sure chunk header+paylaod is not larger than the total compressed */
        if (chunk_offset + length + 12 > upng->source.size)
        {
            SET_ERROR(upng, UPNG_EMALFORMED);
            return upng->error;
        }

        /* parse chunks */
        if (upng_chunk_type(chunk_header) == CHUNK_IDAT)
        {
            compressed_size += length;
        }
        else if (upng_chunk_type(chunk_header) == CHUNK_IEND)
        {
            break;
        }
        else if (upng_chunk_type(chunk_header) == CHUNK_OFFS)
        {
            unsigned char data[8];
            if (upng->source.read(upng->source.user, chunk_data_offset, data, 8) != 8)
            {
                SET_ERROR(upng, UPNG_EREAD);
                return upng->error;
            }

            upng->x_offset = MAKE_DWORD_PTR(data);
            upng->y_offset = MAKE_DWORD_PTR(data + 4);
        }
        else if (upng_chunk_type(chunk_header) == CHUNK_PLTE)
        {
            upng->palette_entries = length / 3; // 3 bytes per color entry
            if (upng->palette)
            {
                app_free(upng->palette);
                upng->palette = NULL;
            }
            upng->palette = app_malloc(length);

            if (upng->source.read(upng->source.user, chunk_data_offset, upng->palette, length) != length)
            {
                SET_ERROR(upng, UPNG_EREAD);
                return upng->error;
            }
        }
        else if (upng_chunk_type(chunk_header) == CHUNK_tRNS)
        {
            upng->alpha_entries = length;
            if (upng->alpha)
            {
                app_free(upng->alpha);
                upng->alpha = NULL;
            }
            upng->alpha = app_malloc(length);

            if (upng->source.read(upng->source.user, chunk_data_offset, upng->alpha, length) != length)
            {
                SET_ERROR(upng, UPNG_EREAD);
                return upng->error;
            }
        }
        else if (upng_chunk_type(chunk_header) == CHUNK_TEXT)
        {
            /*int keyword_length = (strlen((const char *)data) + 1);
            // Copy keyword located at start of data (includes null terminator)
            upng->text[upng->text_count].keyword = app_malloc(keyword_length);
            strcpy(upng->text[upng->text_count].keyword, (const char *)data);

            int text_length = length - keyword_length + 1;
            // Copy the text from data, starts after the null after keyword
            upng->text[upng->text_count].text = app_malloc(text_length);
            memcpy((char *)upng->text[upng->text_count].text, (const char *)(data + keyword_length), text_length - 1); // no null terminator
            // add missing null terminator
            upng->text[upng->text_count].text[text_length - 1] = '\0';

            upng->text_count++;*/
            // TODO: Reimplement with new source callbacks
        }
        else if (upng_chunk_critical(chunk_header))
        {
            SET_ERROR(upng, UPNG_EUNSUPPORTED);
            return upng->error;
        }

        chunk_offset += length + 12;
    }

    /* allocate enough space for the (compressed and filtered) image data */
    compressed = (unsigned char *)app_malloc(compressed_size);
    if (compressed == NULL)
    {
        SET_ERROR(upng, UPNG_ENOMEM);
        return upng->error;
    }

    /* scan through the chunks again, this time copying the values into
        * our compressed buffer.  there's no reason to validate anything a second time. */
    chunk_offset = 33;
    while (chunk_offset < upng->source.size)
    {
        unsigned long chunk_data_offset = upng_chunk_data(chunk_offset);
        unsigned long length;

        /* read chunk header */
        if (upng->source.read(upng->source.user, chunk_offset, chunk_header, 12) != 12)
        {
            SET_ERROR(upng, UPNG_EREAD);
            return upng->error;
        }

        length = upng_chunk_length(chunk_header);

        /* parse chunks */
        if (upng_chunk_type(chunk_header) == CHUNK_IDAT)
        {
            if (upng->source.read(upng->source.user, chunk_data_offset, compressed + compressed_index, length) != length)
            {
                SET_ERROR(upng, UPNG_EREAD);
                return upng->error;
            }
            compressed_index += length;
        }
        else if (upng_chunk_type(chunk_header) == CHUNK_IEND)
        {
            break;
        }

        chunk_offset += length + 12;
    }

    /* allocate space to store inflated (but still filtered) data */
    int width_aligned_bytes = (upng->width * upng_get_bpp(upng) + 7) / 8;
    inflated_size = (width_aligned_bytes * upng->height) + upng->height; // pad byte
                                                                         // Hard-codec CCM usage, avoid compositor buffer (ie. +32k to be safe)
    inflated = (unsigned char *)app_malloc(inflated_size);
    if (inflated == NULL)
    {
        app_free(compressed);
        SET_ERROR(upng, UPNG_ENOMEM);
        return upng->error;
    }

    /* decompress image data */
    error = uz_inflate(inflated, inflated_size, compressed, compressed_size);
    if (error != UPNG_EOK)
    {
        app_free(inflated);
        app_free(compressed);
        return upng->error;
    }

    /* free the compressed compressed data */
    app_free(compressed);

    /* allocate final image buffer */
    upng->size = width_aligned_bytes * upng->height;

    /* unfilter scanlines */
    post_process_scanlines(upng, inflated, inflated, upng);
    upng->buffer = inflated;

    if (upng->error != UPNG_EOK)
    {
        app_free(upng->buffer);
        upng->buffer = NULL;
        upng->size = 0;
    }
    else
    {
        upng->state = UPNG_DECODED;
    }

    /* we are done with our input buffer; free it if we own it */
    upng_free_source(upng);

    return upng->error;
}

upng_t *upng_new_from_source(upng_source source)
{
    upng_t *upng;

    upng = (upng_t *)app_malloc(sizeof(upng_t));
    if (upng == NULL)
    {
        return NULL;
    }

    upng->buffer = NULL;
    upng->size = 0;

    upng->width = upng->height = 0;

    upng->x_offset = 0;
    upng->y_offset = 0;

    upng->palette = NULL;
    upng->palette_entries = 0;

    upng->alpha = NULL;
    upng->alpha_entries = 0;

    upng->color_type = UPNG_RGBA;
    upng->color_depth = 8;
    upng->format = UPNG_RGBA8;

    upng->state = UPNG_NEW;

    upng->error = UPNG_EOK;
    upng->error_line = 0;

    upng->text_count = 0;

    upng->source = source;

    return upng;
}

typedef struct upng_byte_source_context
{
    void* buffer;
    unsigned long size;
} upng_byte_source_context;

static unsigned long upng_byte_source_read(void* user, unsigned long offset, void* out_buffer, unsigned long read_size)
{
    upng_byte_source_context* context = (upng_byte_source_context*)user;
    if (offset >= context->size)
        return 0;

    unsigned long bytes_to_copy = read_size;
    if (offset + bytes_to_copy > context->size)
        bytes_to_copy = context->size - offset;

    memcpy(out_buffer, context->buffer + offset, bytes_to_copy);
    return bytes_to_copy;
}

static void upng_byte_source_free(void* user)
{
    app_free(user);
}

upng_t *upng_new_from_bytes(unsigned char *raw_buffer, unsigned long size, uint8_t **out_buffer)
{
    upng_byte_source_context* context = (upng_byte_source_context*)app_malloc(sizeof(upng_byte_source_context));
    if (context == NULL)
        return NULL;
    context->buffer = raw_buffer;
    context->size = size;

    return upng_new_from_source((upng_source) {
        .user = context,
        .size = size,
        .read = upng_byte_source_read,
        .free = upng_byte_source_free
    });
}

#ifdef UPNG_USE_STDIO
static unsigned long upng_file_source_read(void* user, unsigned long offset, void* out_buffer, unsigned long read_size)
{
    FILE* fp = (FILE*)user;
    fseek(fp, 0, SEEK_END);
    unsigned long size = ftell(fp);
    if (offset >= size)
        return 0;

    unsigned long bytes_to_read = read_size;
    if (offset + bytes_to_read > size)
        bytes_to_read = size - offset;

    fseek(fp, offset, SEEK_SET);
    return fread(out_buffer, 1, bytes_to_read, fp);
}

static void upng_file_source_free(void* user)
{
    if (user != NULL)
        fclose((FILE*)user);
}

upng_t *upng_new_from_file(const char *filename)
{
    FILE* fp = fopen(filename, "rb");
    upng_t *upng = upng_new_from_source((upng_source) {
        .user = fp,
        .size = 0,
        .read = upng_file_source_read,
        .free = upng_file_source_free
    });
    if (upng == NULL)
        return NULL;

    if (upng->source.user == NULL)
    {
        SET_ERROR(upng, UPNG_ENOTFOUND);
        return upng;
    }

    fseek(fp, 0, SEEK_END);
    upng->source.size = ftell(fp);
    return upng;
}
#endif

void upng_free(upng_t *upng)
{
    /* We don't deallocate upng->buffer, because that gets handed off to the
     * user (in this case, png_to_gbitmap).  */

    /* deallocate palette buffer, if necessary */
    if (upng->palette)
    {
        app_free(upng->palette);
    }

    /* deallocate alpha buffer, we rolled all alphas into the palette */
    if (upng->alpha)
    {
        app_free(upng->alpha);
    }

    /* deallocate source buffer, if necessary */
    upng_free_source(upng);

    if (upng->text_count)
    {
        for (unsigned int i = 0; i < upng->text_count; i++)
        {
            app_free(upng->text[i].keyword);
            app_free(upng->text[i].text);
        }
    }
    upng->text_count = 0;

    /* deallocate struct itself */
    app_free(upng);
}

upng_error upng_get_error(const upng_t *upng)
{
    return upng->error;
}

unsigned upng_get_error_line(const upng_t *upng)
{
    return upng->error_line;
}

unsigned upng_get_width(const upng_t *upng)
{
    return upng->width;
}

unsigned upng_get_height(const upng_t *upng)
{
    return upng->height;
}

int upng_get_x_offset(const upng_t *upng)
{
    return upng->x_offset;
}

int upng_get_y_offset(const upng_t *upng)
{
    return upng->y_offset;
}

int upng_get_palette(const upng_t *upng, rgb **palette)
{
    *palette = upng->palette;
    return upng->palette_entries;
}

int upng_get_alpha(const upng_t *upng, uint8_t **alpha)
{
    *alpha = upng->alpha;
    return upng->alpha_entries;
}

unsigned upng_get_bpp(const upng_t *upng)
{
    return upng_get_bitdepth(upng) * upng_get_components(upng);
}

unsigned upng_get_components(const upng_t *upng)
{
    switch (upng->color_type)
    {
    case UPNG_PLT:
        return 1;
    case UPNG_LUM:
        return 1;
    case UPNG_RGB:
        return 3;
    case UPNG_LUMA:
        return 2;
    case UPNG_RGBA:
        return 4;
    default:
        return 0;
    }
}

unsigned upng_get_bitdepth(const upng_t *upng)
{
    return upng->color_depth;
}

upng_format upng_get_format(const upng_t *upng)
{
    return upng->format;
}

char *upng_get_text(const upng_t *upng, char **text_out, unsigned int index)
{
    if (index < upng->text_count)
    {
        *text_out = upng->text[index].text;
        return upng->text[index].keyword;
    }
    return NULL;
}

const unsigned char *upng_get_buffer(const upng_t *upng)
{
    return upng->buffer;
}

unsigned upng_get_size(const upng_t *upng)
{
    return upng->size;
}
