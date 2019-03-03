/*
auPNG -- derived from LodePNG version 20100808

Copyright (c) 2005-2010 Lode Vandevenne
Copyright (c) 2010 Sean Middleditch
Copyright (c) 2019 Helco

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

#if !defined(UPNG_H)
#define UPNG_H
#include <pebble.h>

typedef enum upng_error {
	UPNG_EOK			= 0, /* success (no error) */
	UPNG_ENOMEM			= 1, /* memory allocation failed */
	UPNG_ENOTFOUND		= 2, /* resource not found (file missing) */
	UPNG_ENOTPNG		= 3, /* image data does not have a PNG header */
	UPNG_EMALFORMED		= 4, /* image data is not a valid PNG image */
	UPNG_EUNSUPPORTED	= 5, /* critical PNG chunk type is not supported */
	UPNG_EUNINTERLACED	= 6, /* image interlacing is not supported */
	UPNG_EUNFORMAT		= 7, /* image color format is not supported */
	UPNG_EPARAM			= 8, /* invalid parameter to method call */
    UPNG_EREAD          = 9  /* read callback did not return all data */
} upng_error;

typedef enum upng_format {
	UPNG_BADFORMAT,
  UPNG_INDEXED1,
  UPNG_INDEXED2,
  UPNG_INDEXED4,
  UPNG_INDEXED8,
	UPNG_RGB8,
	UPNG_RGB16,
	UPNG_RGBA8,
	UPNG_RGBA16,
	UPNG_LUMINANCE1,
	UPNG_LUMINANCE2,
	UPNG_LUMINANCE4,
	UPNG_LUMINANCE8,
	UPNG_LUMINANCE_ALPHA1,
	UPNG_LUMINANCE_ALPHA2,
	UPNG_LUMINANCE_ALPHA4,
	UPNG_LUMINANCE_ALPHA8
} upng_format;

typedef struct upng_t upng_t;

typedef struct __attribute__((__packed__)) rgb {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} rgb;

typedef void (*upng_source_free_cb)(void* user);
typedef unsigned long (*upng_source_read_cb)(void* user, unsigned long offset, void* buffer, unsigned long size);
typedef struct upng_source
{
    void* user;
    unsigned long size;
    upng_source_free_cb free;
    upng_source_read_cb read;
} upng_source;

#ifdef UPNG_USE_STDIO
upng_t*		upng_new_from_file	 (const char* path);
#endif
upng_t*		upng_new_from_bytes	 (unsigned char* source_buffer, unsigned long source_size, unsigned char**buffer);
upng_t*     upng_new_from_source (upng_source source);
void		upng_free			 (upng_t* upng);

upng_error	upng_header			 (upng_t* upng);
upng_error	upng_decode			 (upng_t* upng);

upng_error	upng_get_error		 (const upng_t* upng);
unsigned	upng_get_error_line	 (const upng_t* upng);

unsigned	upng_get_width		 (const upng_t* upng);
unsigned	upng_get_height		 (const upng_t* upng);
int	        upng_get_x_offset	 (const upng_t* upng);
int	        upng_get_y_offset	 (const upng_t* upng);
unsigned	upng_get_bpp		 (const upng_t* upng);
unsigned	upng_get_bitdepth	 (const upng_t* upng);
unsigned	upng_get_components	 (const upng_t* upng);
upng_format	upng_get_format		 (const upng_t* upng);

//returns count of entries in palette
int         upng_get_palette(const upng_t* upng, rgb **palette);

const unsigned char*	upng_get_buffer		(const upng_t* upng);
unsigned				upng_get_size		(const upng_t* upng);

//returns keyword and text_out matching keyword
const char* upng_get_text(const upng_t* upng, const char** text_out, unsigned int index);
int         upng_get_alpha(const upng_t* upng, uint8_t **alpha);

#endif /*defined(UPNG_H)*/
