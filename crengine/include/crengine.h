/**
    (c) Vadim Lopatin, 2000-2008

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/
#ifndef CRENGINE_H_INCLUDED
#define CRENGINE_H_INCLUDED

#if defined(_LINUX) || defined (LINUX)
#define USE_LIBJPEG                          1
#define USE_LIBPNG                           1
#define USE_GIF                              1
#define USE_ANSI_FILES                       1
#define GRAY_INVERSE                         0
#define USE_FREETYPE                         1

#ifndef ANDROID
#ifndef MAC
#ifndef TIZEN
#ifndef USE_FONTCONFIG

#define USE_FONTCONFIG						 1

#endif
#endif
#endif
#endif

#define ALLOW_KERNING                        1
#define GLYPH_CACHE_SIZE                     0x40000
#define ZIP_STREAM_BUFFER_SIZE               0x40000
#define FILE_STREAM_BUFFER_SIZE              0x20000
#endif //defined(_LINUX) || defined (LINUX)

#if (LBOOK==1)
#define USE_DOM_UTF8_STORAGE                 1
#ifndef MAX_IMAGE_SCALE_MUL
#define MAX_IMAGE_SCALE_MUL                  2
#endif
#define USE_ANSI_FILES                       1
#define GRAY_INVERSE                         0
#define ALLOW_KERNING                        1
#define USE_LIBJPEG                          1
#define USE_LIBPNG                           1
#define USE_GIF                              1
#define USE_FREETYPE                         1
#define GLYPH_CACHE_SIZE                     0x20000
#define ZIP_STREAM_BUFFER_SIZE               0x80000
#define FILE_STREAM_BUFFER_SIZE              0x40000
#endif // LBOOK==1

#if defined(_WIN32)
/// maximum picture zoom (1, 2, 3)
#define GRAY_INVERSE						 0
#ifndef MAX_IMAGE_SCALE_MUL
#define MAX_IMAGE_SCALE_MUL                  1
#endif
#if defined(CYGWIN)
#define USE_FREETYPE                         0
#else
#define USE_FREETYPE                         1
#endif
#define ALLOW_KERNING                        1
#define GLYPH_CACHE_SIZE                     0x20000
#define ZIP_STREAM_BUFFER_SIZE               0x80000
#define FILE_STREAM_BUFFER_SIZE              0x40000
//#define USE_LIBJPEG 0
#endif // defined(_WIN32)

#ifndef GLYPH_CACHE_SIZE
/// freetype font glyph buffer size, in bytes
#define GLYPH_CACHE_SIZE 0x40000
#endif

#ifndef USE_GIF
///allow GIF support via embedded decoder
#define USE_GIF 1
#endif

#ifndef USE_LIBJPEG
///allow JPEG support via libjpeg
#define USE_LIBJPEG 1
#endif

#ifndef USE_LIBPNG
///allow PNG support via libpng
#define USE_LIBPNG 1
#endif

#ifndef GRAY_INVERSE
#define GRAY_INVERSE 1
#endif

/** \def LVLONG_FILE_SUPPORT
    \brief define to 1 to use 64 bits for file position types
*/
#define LVLONG_FILE_SUPPORT 0

//#define USE_ANSI_FILES 1

/// zlib stream decode cache size, used to avoid restart of decoding from beginning to move back
#ifndef ZIP_STREAM_BUFFER_SIZE
#define ZIP_STREAM_BUFFER_SIZE 0x10000
#endif

/// document stream buffer size
#ifndef FILE_STREAM_BUFFER_SIZE
#define FILE_STREAM_BUFFER_SIZE 0x40000
#endif

#if (USE_FREETYPE!=1)

#ifndef ALLOW_KERNING
/// set to 1 to allow kerning
#define ALLOW_KERNING 0
#endif

#endif

#ifndef USE_FREETYPE
#define USE_FREETYPE 0
#endif

#ifndef USE_DOM_UTF8_STORAGE
#define USE_DOM_UTF8_STORAGE 0
#endif

#ifndef USE_BITMAP_FONTS

#if (USE_FREETYPE==1)
#define USE_BITMAP_FONTS 0
#else
#define USE_BITMAP_FONTS 1
#endif

#endif

// max unpacked size of skin image to hold in cache unpacked
#ifndef MAX_SKIN_IMAGE_CACHE_ITEM_UNPACKED_SIZE
#define MAX_SKIN_IMAGE_CACHE_ITEM_UNPACKED_SIZE 80*80*4
#endif

// max skin image file size to hold as a packed copy in memory
#ifndef MAX_SKIN_IMAGE_CACHE_ITEM_RAM_COPY_PACKED_SIZE
#define MAX_SKIN_IMAGE_CACHE_ITEM_RAM_COPY_PACKED_SIZE 10000
#endif

#ifndef ENABLE_ANTIWORD
#define ENABLE_ANTIWORD 1
#endif

#endif //CRENGINE_H_INCLUDED