// stb_truetype.h - v1.26 - public domain
// authored from 2009-2021 by Sean Barrett / RAD Game Tools
//
// =======================================================================
//
//    NO SECURITY GUARANTEE -- DO NOT USE THIS ON UNTRUSTED FONT FILES
//
// This library does no range checking of the offsets found in the file,
// meaning an attacker can use it to read arbitrary memory.
//
// =======================================================================
//
//   This library processes TrueType files:
//        parse files
//        extract glyph metrics
//        extract glyph shapes
//        render glyphs to one-channel bitmaps with antialiasing (box filter)
//        render glyphs to one-channel SDF bitmaps (signed-distance field/function)
//
//   Todo:
//        non-MS cmaps
//        crashproof on bad data
//        hinting? (no longer patented)
//        cleartype-style AA?
//        optimize: use simple memory allocator for intermediates
//        optimize: build edge-list directly from curves
//        optimize: rasterize directly from curves?
//
// ADDITIONAL CONTRIBUTORS
//
//   Mikko Mononen: compound shape support, more cmap formats
//   Tor Andersson: kerning, subpixel rendering
//   Dougall Johnson: OpenType / Type 2 font handling
//   Daniel Ribeiro Maciel: basic GPOS-based kerning
//
//   Misc other:
//       Ryan Gordon
//       Simon Glass
//       github:IntellectualKitty
//       Imanol Celaya
//       Daniel Ribeiro Maciel
//
//   Bug/warning reports/fixes:
//       "Zer" on mollyrocket       Fabian "ryg" Giesen   github:NiLuJe
//       Cass Everitt               Martins Mozeiko       github:aloucks
//       stoiko (Haemimont Games)   Cap Petschulat        github:oyvindjam
//       Brian Hook                 Omar Cornut           github:vassvik
//       Walter van Niftrik         Ryan Griege
//       David Gow                  Peter LaValle
//       David Given                Sergey Popov
//       Ivan-Assen Ivanov          Giumo X. Clanjor
//       Anthony Pesch              Higor Euripedes
//       Johan Duparc               Thomas Fields
//       Hou Qiming                 Derek Vinyard
//       Rob Loach                  Cort Stratton
//       Kenney Phillis Jr.         Brian Costabile
//       Ken Voskuil (kaesve)
//
// LICENSE
//
//   See end of file for license information.
//

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
////
////   INTEGRATION WITH YOUR CODEBASE
////
////   The following sections allow you to supply alternate definitions
////   of C library functions used by stb_truetype, e.g. if you don't
////   link with the C runtime library.

#ifdef STB_TRUETYPE_IMPLEMENTATION
// #define your own (u)stbtt_int8/16/32 before including to override this
#ifndef stbtt_uint8
typedef unsigned char stbtt_uint8;
typedef signed char stbtt_int8;
typedef unsigned short stbtt_uint16;
typedef signed short stbtt_int16;
typedef unsigned int stbtt_uint32;
typedef signed int stbtt_int32;
#endif

typedef char stbtt__check_size32[sizeof(stbtt_int32) == 4 ? 1 : -1];
typedef char stbtt__check_size16[sizeof(stbtt_int16) == 2 ? 1 : -1];

// e.g. #define your own STBTT_ifloor/STBTT_iceil() to avoid math.h
#ifndef STBTT_ifloor
#include <math.h>
#define STBTT_ifloor(x) ((int)floor(x))
#define STBTT_iceil(x) ((int)ceil(x))
#endif

#ifndef STBTT_sqrt
#include <math.h>
#define STBTT_sqrt(x) sqrt(x)
#define STBTT_pow(x, y) pow(x, y)
#endif

#ifndef STBTT_fmod
#include <math.h>
#define STBTT_fmod(x, y) fmod(x, y)
#endif

#ifndef STBTT_cos
#include <math.h>
#define STBTT_cos(x) cos(x)
#define STBTT_acos(x) acos(x)
#endif

#ifndef STBTT_fabs
#include <math.h>
#define STBTT_fabs(x) fabs(x)
#endif

// #define your own functions "STBTT_malloc" / "STBTT_free" to avoid malloc.h
#ifndef STBTT_malloc
#include <stdlib.h>
#define STBTT_malloc(x, u) ((void)(u), malloc(x))
#define STBTT_free(x, u) ((void)(u), free(x))
#endif

#ifndef STBTT_assert
#include <assert.h>
#define STBTT_assert(x) assert(x)
#endif

#ifndef STBTT_strlen
#include <string.h>
#define STBTT_strlen(x) strlen(x)
#endif

#ifndef STBTT_memcpy
#include <string.h>
#define STBTT_memcpy memcpy
#define STBTT_memset memset
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////
////   INTERFACE
////
////

#ifndef __STB_INCLUDE_STB_TRUETYPE_H__
#define __STB_INCLUDE_STB_TRUETYPE_H__

#ifdef STBTT_STATIC
#define STBTT_DEF static
#else
#define STBTT_DEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

// private structure
typedef struct {
  unsigned char *data;
  int cursor;
  int size;
} stbtt__buf;

//////////////////////////////////////////////////////////////////////////////
//
// FONT LOADING
//
//

// The following structure is defined publicly so you can declare one on
// the stack or as a global or etc, but you should treat it as opaque.
struct stbtt_fontinfo {
  void *userdata;
  unsigned char *data; // pointer to .ttf file
  int fontstart;       // offset of start of font

  int numGlyphs; // number of glyphs, needed for range checking

  int loca, head, glyf, hhea, hmtx, kern, gpos, svg; // table locations as offset from start of .ttf
  int index_map;                                     // a cmap mapping for our chosen character encoding
  int indexToLocFormat;                              // format needed to map from glyph index to glyph

  stbtt__buf cff;         // cff font data
  stbtt__buf charstrings; // the charstring index
  stbtt__buf gsubrs;      // global charstring subroutines index
  stbtt__buf subrs;       // private charstring subroutines index
  stbtt__buf fontdicts;   // array of font dicts
  stbtt__buf fdselect;    // map from glyph to fontdict
};

STBTT_DEF int stbtt_InitFont(stbtt_fontinfo *info, const unsigned char *data, int offset);
// Given an offset into the file that defines a font, this function builds
// the necessary cached info for the rest of the system. You must allocate
// the stbtt_fontinfo yourself, and stbtt_InitFont will fill it out. You don't
// need to do anything special to free it, because the contents are pure
// value data with no additional data structures. Returns 0 on failure.

//////////////////////////////////////////////////////////////////////////////
//
// CHARACTER TO GLYPH-INDEX CONVERSIOn

STBTT_DEF int stbtt_FindGlyphIndex(const stbtt_fontinfo *info, int unicode_codepoint);
// If you're going to perform multiple operations on the same character
// and you want a speed-up, call this function with the character you're
// going to process, then use glyph-based functions instead of the
// codepoint-based functions.
// Returns 0 if the character codepoint is not defined in the font.

//////////////////////////////////////////////////////////////////////////////
//
// CHARACTER PROPERTIES
//

STBTT_DEF float stbtt_ScaleForMappingEmToPixels(const stbtt_fontinfo *info, float pixels);
// computes a scale factor to produce a font whose EM size is mapped to
// 'pixels' tall. This is probably what traditional APIs compute, but
// I'm not positive.

STBTT_DEF void stbtt_GetFontVMetrics(const stbtt_fontinfo *info, int *ascent, int *descent, int *lineGap);
// ascent is the coordinate above the baseline the font extends; descent
// is the coordinate below the baseline the font extends (i.e. it is typically negative)
// lineGap is the spacing between one row's descent and the next row's ascent...
// so you should advance the vertical position by "*ascent - *descent + *lineGap"
//   these are expressed in unscaled coordinates, so you must multiply by
//   the scale factor for a given size

#if 0
STBTT_DEF int  stbtt_GetFontVMetricsOS2(const stbtt_fontinfo *info, int *typoAscent, int *typoDescent, int *typoLineGap);
// analogous to GetFontVMetrics, but returns the "typographic" values from the OS/2
// table (specific to MS/Windows TTF files).
//
// Returns 1 on success (table present), 0 on failure.

STBTT_DEF void stbtt_GetFontBoundingBox(const stbtt_fontinfo *info, int *x0, int *y0, int *x1, int *y1);
// the bounding box around all possible characters

STBTT_DEF void stbtt_GetCodepointHMetrics(const stbtt_fontinfo *info, int codepoint, int *advanceWidth, int *leftSideBearing);
// leftSideBearing is the offset from the current horizontal position to the left edge of the character
// advanceWidth is the offset from the current horizontal position to the next horizontal position
//   these are expressed in unscaled coordinates

STBTT_DEF int  stbtt_GetCodepointKernAdvance(const stbtt_fontinfo *info, int ch1, int ch2);
// an additional amount to add to the 'advance' value between ch1 and ch2

STBTT_DEF int stbtt_GetCodepointBox(const stbtt_fontinfo *info, int codepoint, int *x0, int *y0, int *x1, int *y1);
// Gets the bounding box of the visible part of the glyph, in unscaled coordinates
#endif

STBTT_DEF void stbtt_GetGlyphHMetrics(const stbtt_fontinfo *info, int glyph_index, int *advanceWidth, int *leftSideBearing);
STBTT_DEF int stbtt_GetGlyphKernAdvance(const stbtt_fontinfo *info, int glyph1, int glyph2);
STBTT_DEF int stbtt_GetGlyphBox(const stbtt_fontinfo *info, int glyph_index, int *x0, int *y0, int *x1, int *y1);
// as above, but takes one or more glyph indices for greater efficiency

typedef struct stbtt_kerningentry {
  int glyph1; // use stbtt_FindGlyphIndex
  int glyph2;
  int advance;
} stbtt_kerningentry;

STBTT_DEF int stbtt_GetKerningTableLength(const stbtt_fontinfo *info);
STBTT_DEF int stbtt_GetKerningTable(const stbtt_fontinfo *info, stbtt_kerningentry *table, int table_length);
// Retrieves a complete list of all of the kerning pairs provided by the font
// stbtt_GetKerningTable never writes more than table_length entries and returns how many entries it did write.
// The table will be sorted by (a.glyph1 == b.glyph1)?(a.glyph2 < b.glyph2):(a.glyph1 < b.glyph1)

//////////////////////////////////////////////////////////////////////////////
//
// GLYPH SHAPES (you probably don't need these, but they have to go before
// the bitmaps for C declaration-order reasons)
//

#ifndef STBTT_vmove // you can predefine these to use different values (but why?)
enum { STBTT_vmove = 1, STBTT_vline, STBTT_vcurve, STBTT_vcubic };
#endif

#ifndef stbtt_vertex            // you can predefine this to use different values
                                // (we share this with other code at RAD)
#define stbtt_vertex_type short // can't use stbtt_int16 because that's not visible in the header file
typedef struct {
  stbtt_vertex_type x, y, cx, cy, cx1, cy1;
  unsigned char type, padding;
} stbtt_vertex;
#endif

STBTT_DEF int stbtt_IsGlyphEmpty(const stbtt_fontinfo *info, int glyph_index);
// returns non-zero if nothing is drawn for this glyph

STBTT_DEF int stbtt_GetGlyphShape(const stbtt_fontinfo *info, int glyph_index, stbtt_vertex **vertices);
// returns # of vertices and fills *vertices with the pointer to them
//   these are expressed in "unscaled" coordinates
//
// The shape is a series of contours. Each one starts with
// a STBTT_moveto, then consists of a series of mixed
// STBTT_lineto and STBTT_curveto segments. A lineto
// draws a line from previous endpoint to its x,y; a curveto
// draws a quadratic bezier from previous endpoint to
// its x,y, using cx,cy as the bezier control point.

#if 0
STBTT_DEF void stbtt_FreeShape(const stbtt_fontinfo *info, stbtt_vertex *vertices);
// frees the data allocated above
#endif

STBTT_DEF void stbtt_GetGlyphBitmapBoxSubpixel(
  const stbtt_fontinfo *font,
  int glyph,
  float scale_x,
  float scale_y,
  float shift_x,
  float shift_y,
  int *ix0,
  int *iy0,
  int *ix1,
  int *iy1);

//////////////////////////////////////////////////////////////////////////////
//
// Signed Distance Function (or Field) rendering

STBTT_DEF void stbtt_FreeSDF(unsigned char *bitmap, void *userdata);
// frees the SDF bitmap allocated below

STBTT_DEF unsigned char *stbtt_GetGlyphSDF(
  const stbtt_fontinfo *info,
  float scale,
  int glyph,
  int padding,
  unsigned char onedge_value,
  float pixel_dist_scale,
  int *width,
  int *height,
  int *xoff,
  int *yoff);
STBTT_DEF unsigned char *stbtt_GetCodepointSDF(
  const stbtt_fontinfo *info,
  float scale,
  int codepoint,
  int padding,
  unsigned char onedge_value,
  float pixel_dist_scale,
  int *width,
  int *height,
  int *xoff,
  int *yoff);
// These functions compute a discretized SDF field for a single character, suitable for storing
// in a single-channel texture, sampling with bilinear filtering, and testing against
// larger than some threshold to produce scalable fonts.
//        info              --  the font
//        scale             --  controls the size of the resulting SDF bitmap, same as it would be creating a regular bitmap
//        glyph/codepoint   --  the character to generate the SDF for
//        padding           --  extra "pixels" around the character which are filled with the distance to the character (not 0),
//                                 which allows effects like bit outlines
//        onedge_value      --  value 0-255 to test the SDF against to reconstruct the character (i.e. the isocontour of the
//        character) pixel_dist_scale  --  what value the SDF should increase by when moving one SDF "pixel" away from the edge
//        (on the 0..255 scale)
//                                 if positive, > onedge_value is inside; if negative, < onedge_value is inside
//        width,height      --  output height & width of the SDF bitmap (including padding)
//        xoff,yoff         --  output origin of the character
//        return value      --  a 2D array of bytes 0..255, width*height in size
//
// pixel_dist_scale & onedge_value are a scale & bias that allows you to make
// optimal use of the limited 0..255 for your application, trading off precision
// and special effects. SDF values outside the range 0..255 are clamped to 0..255.
//
// Example:
//      scale = stbtt_ScaleForPixelHeight(22)
//      padding = 5
//      onedge_value = 180
//      pixel_dist_scale = 180/5.0 = 36.0
//
//      This will create an SDF bitmap in which the character is about 22 pixels
//      high but the whole bitmap is about 22+5+5=32 pixels high. To produce a filled
//      shape, sample the SDF at each pixel and fill the pixel if the SDF value
//      is greater than or equal to 180/255. (You'll actually want to antialias,
//      which is beyond the scope of this example.) Additionally, you can compute
//      offset outlines (e.g. to stroke the character border inside & outside,
//      or only outside). For example, to fill outside the character up to 3 SDF
//      pixels, you would compare against (180-36.0*3)/255 = 72/255. The above
//      choice of variables maps a range from 5 pixels outside the shape to
//      2 pixels inside the shape to 0..255; this is intended primarily for apply
//      outside effects only (the interior range is needed to allow proper
//      antialiasing of the font at *smaller* sizes)
//
// The function computes the SDF analytically at each SDF pixel, not by e.g.
// building a higher-res bitmap and approximating it. In theory the quality
// should be as high as possible for an SDF of this size & representation, but
// unclear if this is true in practice (perhaps building a higher-res bitmap
// and computing from that can allow drop-out prevention).
//
// The algorithm has not been optimized at all, so expect it to be slow
// if computing lots of characters or very large sizes.

enum { // platformID
  STBTT_PLATFORM_ID_UNICODE = 0,
  STBTT_PLATFORM_ID_MAC = 1,
  STBTT_PLATFORM_ID_ISO = 2,
  STBTT_PLATFORM_ID_MICROSOFT = 3
};

enum { // encodingID for STBTT_PLATFORM_ID_UNICODE
  STBTT_UNICODE_EID_UNICODE_1_0 = 0,
  STBTT_UNICODE_EID_UNICODE_1_1 = 1,
  STBTT_UNICODE_EID_ISO_10646 = 2,
  STBTT_UNICODE_EID_UNICODE_2_0_BMP = 3,
  STBTT_UNICODE_EID_UNICODE_2_0_FULL = 4
};

enum { // encodingID for STBTT_PLATFORM_ID_MICROSOFT
  STBTT_MS_EID_SYMBOL = 0,
  STBTT_MS_EID_UNICODE_BMP = 1,
  STBTT_MS_EID_SHIFTJIS = 2,
  STBTT_MS_EID_UNICODE_FULL = 10
};

enum { // encodingID for STBTT_PLATFORM_ID_MAC; same as Script Manager codes
  STBTT_MAC_EID_ROMAN = 0,
  STBTT_MAC_EID_ARABIC = 4,
  STBTT_MAC_EID_JAPANESE = 1,
  STBTT_MAC_EID_HEBREW = 5,
  STBTT_MAC_EID_CHINESE_TRAD = 2,
  STBTT_MAC_EID_GREEK = 6,
  STBTT_MAC_EID_KOREAN = 3,
  STBTT_MAC_EID_RUSSIAN = 7
};

enum { // languageID for STBTT_PLATFORM_ID_MICROSOFT; same as LCID...
       // problematic because there are e.g. 16 english LCIDs and 16 arabic LCIDs
  STBTT_MS_LANG_ENGLISH = 0x0409,
  STBTT_MS_LANG_ITALIAN = 0x0410,
  STBTT_MS_LANG_CHINESE = 0x0804,
  STBTT_MS_LANG_JAPANESE = 0x0411,
  STBTT_MS_LANG_DUTCH = 0x0413,
  STBTT_MS_LANG_KOREAN = 0x0412,
  STBTT_MS_LANG_FRENCH = 0x040c,
  STBTT_MS_LANG_RUSSIAN = 0x0419,
  STBTT_MS_LANG_GERMAN = 0x0407,
  STBTT_MS_LANG_SPANISH = 0x0409,
  STBTT_MS_LANG_HEBREW = 0x040d,
  STBTT_MS_LANG_SWEDISH = 0x041D
};

enum { // languageID for STBTT_PLATFORM_ID_MAC
  STBTT_MAC_LANG_ENGLISH = 0,
  STBTT_MAC_LANG_JAPANESE = 11,
  STBTT_MAC_LANG_ARABIC = 12,
  STBTT_MAC_LANG_KOREAN = 23,
  STBTT_MAC_LANG_DUTCH = 4,
  STBTT_MAC_LANG_RUSSIAN = 32,
  STBTT_MAC_LANG_FRENCH = 1,
  STBTT_MAC_LANG_SPANISH = 6,
  STBTT_MAC_LANG_GERMAN = 2,
  STBTT_MAC_LANG_SWEDISH = 5,
  STBTT_MAC_LANG_HEBREW = 10,
  STBTT_MAC_LANG_CHINESE_SIMPLIFIED = 33,
  STBTT_MAC_LANG_ITALIAN = 3,
  STBTT_MAC_LANG_CHINESE_TRAD = 19
};

#ifdef __cplusplus
}
#endif

#endif // __STB_INCLUDE_STB_TRUETYPE_H__

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////
////   IMPLEMENTATION
////
////

#ifdef STB_TRUETYPE_IMPLEMENTATION

#ifdef _MSC_VER
#define STBTT__NOTUSED(v) (void)(v)
#else
#define STBTT__NOTUSED(v) (void)sizeof(v)
#endif

//////////////////////////////////////////////////////////////////////////
//
// stbtt__buf helpers to parse data from file
//

static stbtt_uint8 stbtt__buf_get8(stbtt__buf *b) {
  if (b->cursor >= b->size) return 0;
  return b->data[b->cursor++];
}

static stbtt_uint8 stbtt__buf_peek8(stbtt__buf *b) {
  if (b->cursor >= b->size) return 0;
  return b->data[b->cursor];
}

static void stbtt__buf_seek(stbtt__buf *b, int o) {
  STBTT_assert(!(o > b->size || o < 0));
  b->cursor = (o > b->size || o < 0) ? b->size : o;
}

static void stbtt__buf_skip(stbtt__buf *b, int o) { stbtt__buf_seek(b, b->cursor + o); }

static stbtt_uint32 stbtt__buf_get(stbtt__buf *b, int n) {
  stbtt_uint32 v = 0;
  int i;
  STBTT_assert(n >= 1 && n <= 4);
  for (i = 0; i < n; i++) v = (v << 8) | stbtt__buf_get8(b);
  return v;
}

static stbtt__buf stbtt__new_buf(const void *p, size_t size) {
  stbtt__buf r;
  STBTT_assert(size < 0x40000000);
  r.data = (stbtt_uint8 *)p;
  r.size = (int)size;
  r.cursor = 0;
  return r;
}

#define stbtt__buf_get16(b) stbtt__buf_get((b), 2)
#define stbtt__buf_get32(b) stbtt__buf_get((b), 4)

static stbtt__buf stbtt__buf_range(const stbtt__buf *b, int o, int s) {
  stbtt__buf r = stbtt__new_buf(NULL, 0);
  if (o < 0 || s < 0 || o > b->size || s > b->size - o) return r;
  r.data = b->data + o;
  r.size = s;
  return r;
}

static stbtt__buf stbtt__cff_get_index(stbtt__buf *b) {
  int count, start, offsize;
  start = b->cursor;
  count = stbtt__buf_get16(b);
  if (count) {
    offsize = stbtt__buf_get8(b);
    STBTT_assert(offsize >= 1 && offsize <= 4);
    stbtt__buf_skip(b, offsize * count);
    stbtt__buf_skip(b, stbtt__buf_get(b, offsize) - 1);
  }
  return stbtt__buf_range(b, start, b->cursor - start);
}

static stbtt_uint32 stbtt__cff_int(stbtt__buf *b) {
  int b0 = stbtt__buf_get8(b);
  if (b0 >= 32 && b0 <= 246)
    return b0 - 139;
  else if (b0 >= 247 && b0 <= 250)
    return (b0 - 247) * 256 + stbtt__buf_get8(b) + 108;
  else if (b0 >= 251 && b0 <= 254)
    return -(b0 - 251) * 256 - stbtt__buf_get8(b) - 108;
  else if (b0 == 28)
    return stbtt__buf_get16(b);
  else if (b0 == 29)
    return stbtt__buf_get32(b);
  STBTT_assert(0);
  return 0;
}

static void stbtt__cff_skip_operand(stbtt__buf *b) {
  int v, b0 = stbtt__buf_peek8(b);
  STBTT_assert(b0 >= 28);
  if (b0 == 30) {
    stbtt__buf_skip(b, 1);
    while (b->cursor < b->size) {
      v = stbtt__buf_get8(b);
      if ((v & 0xF) == 0xF || (v >> 4) == 0xF) break;
    }
  } else {
    stbtt__cff_int(b);
  }
}

static stbtt__buf stbtt__dict_get(stbtt__buf *b, int key) {
  stbtt__buf_seek(b, 0);
  while (b->cursor < b->size) {
    int start = b->cursor, end, op;
    while (stbtt__buf_peek8(b) >= 28) stbtt__cff_skip_operand(b);
    end = b->cursor;
    op = stbtt__buf_get8(b);
    if (op == 12) op = stbtt__buf_get8(b) | 0x100;
    if (op == key) return stbtt__buf_range(b, start, end - start);
  }
  return stbtt__buf_range(b, 0, 0);
}

static void stbtt__dict_get_ints(stbtt__buf *b, int key, int outcount, stbtt_uint32 *out) {
  int i;
  stbtt__buf operands = stbtt__dict_get(b, key);
  for (i = 0; i < outcount && operands.cursor < operands.size; i++) out[i] = stbtt__cff_int(&operands);
}

static int stbtt__cff_index_count(stbtt__buf *b) {
  stbtt__buf_seek(b, 0);
  return stbtt__buf_get16(b);
}

static stbtt__buf stbtt__cff_index_get(stbtt__buf b, int i) {
  int count, offsize, start, end;
  stbtt__buf_seek(&b, 0);
  count = stbtt__buf_get16(&b);
  offsize = stbtt__buf_get8(&b);
  STBTT_assert(i >= 0 && i < count);
  STBTT_assert(offsize >= 1 && offsize <= 4);
  stbtt__buf_skip(&b, i * offsize);
  start = stbtt__buf_get(&b, offsize);
  end = stbtt__buf_get(&b, offsize);
  return stbtt__buf_range(&b, 2 + (count + 1) * offsize + start, end - start);
}

//////////////////////////////////////////////////////////////////////////
//
// accessors to parse data from file
//

// on platforms that don't allow misaligned reads, if we want to allow
// truetype fonts that aren't padded to alignment, define ALLOW_UNALIGNED_TRUETYPE

#define ttBYTE(p) (*(stbtt_uint8 *)(p))
#define ttCHAR(p) (*(stbtt_int8 *)(p))

static stbtt_uint16 ttUSHORT(stbtt_uint8 *p) { return p[0] * 256 + p[1]; }
static stbtt_int16 ttSHORT(stbtt_uint8 *p) { return p[0] * 256 + p[1]; }
static stbtt_uint32 ttULONG(stbtt_uint8 *p) { return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3]; }

#define stbtt_tag4(p, c0, c1, c2, c3) ((p)[0] == (c0) && (p)[1] == (c1) && (p)[2] == (c2) && (p)[3] == (c3))
#define stbtt_tag(p, str) stbtt_tag4(p, str[0], str[1], str[2], str[3])

#if 0
static int stbtt__isfont(stbtt_uint8 *font)
{
   // check the version number
   if (stbtt_tag4(font, '1',0,0,0))  return 1; // TrueType 1
   if (stbtt_tag(font, "typ1"))   return 1; // TrueType with type 1 font -- we don't support this!
   if (stbtt_tag(font, "OTTO"))   return 1; // OpenType with CFF
   if (stbtt_tag4(font, 0,1,0,0)) return 1; // OpenType 1.0
   if (stbtt_tag(font, "true"))   return 1; // Apple specification for TrueType fonts
   return 0;
}
#endif

// @OPTIMIZE: binary search
static stbtt_uint32 stbtt__find_table(stbtt_uint8 *data, stbtt_uint32 fontstart, const char *tag) {
  stbtt_int32 num_tables = ttUSHORT(data + fontstart + 4);
  stbtt_uint32 tabledir = fontstart + 12;
  stbtt_int32 i;
  for (i = 0; i < num_tables; ++i) {
    stbtt_uint32 loc = tabledir + 16 * i;
    if (stbtt_tag(data + loc + 0, tag)) return ttULONG(data + loc + 8);
  }
  return 0;
}

static stbtt__buf stbtt__get_subrs(stbtt__buf cff, stbtt__buf fontdict) {
  stbtt_uint32 subrsoff = 0, private_loc[2] = {0, 0};
  stbtt__buf pdict;
  stbtt__dict_get_ints(&fontdict, 18, 2, private_loc);
  if (!private_loc[1] || !private_loc[0]) return stbtt__new_buf(NULL, 0);
  pdict = stbtt__buf_range(&cff, private_loc[1], private_loc[0]);
  stbtt__dict_get_ints(&pdict, 19, 1, &subrsoff);
  if (!subrsoff) return stbtt__new_buf(NULL, 0);
  stbtt__buf_seek(&cff, private_loc[1] + subrsoff);
  return stbtt__cff_get_index(&cff);
}

static int stbtt_InitFont_internal(stbtt_fontinfo *info, unsigned char *data, int fontstart) {
  stbtt_uint32 cmap, t;
  stbtt_int32 i, numTables;

  info->data = data;
  info->fontstart = fontstart;
  info->cff = stbtt__new_buf(NULL, 0);

  cmap = stbtt__find_table(data, fontstart, "cmap");       // required
  info->loca = stbtt__find_table(data, fontstart, "loca"); // required
  info->head = stbtt__find_table(data, fontstart, "head"); // required
  info->glyf = stbtt__find_table(data, fontstart, "glyf"); // required
  info->hhea = stbtt__find_table(data, fontstart, "hhea"); // required
  info->hmtx = stbtt__find_table(data, fontstart, "hmtx"); // required
  info->kern = stbtt__find_table(data, fontstart, "kern"); // not required
  info->gpos = stbtt__find_table(data, fontstart, "GPOS"); // not required

  if (!cmap || !info->head || !info->hhea || !info->hmtx) return 0;
  if (info->glyf) {
    // required for truetype
    if (!info->loca) return 0;
  } else {
    // initialization for CFF / Type2 fonts (OTF)
    stbtt__buf b, topdict, topdictidx;
    stbtt_uint32 cstype = 2, charstrings = 0, fdarrayoff = 0, fdselectoff = 0;
    stbtt_uint32 cff;

    cff = stbtt__find_table(data, fontstart, "CFF ");
    if (!cff) return 0;

    info->fontdicts = stbtt__new_buf(NULL, 0);
    info->fdselect = stbtt__new_buf(NULL, 0);

    // @TODO this should use size from table (not 512MB)
    info->cff = stbtt__new_buf(data + cff, 512 * 1024 * 1024);
    b = info->cff;

    // read the header
    stbtt__buf_skip(&b, 2);
    stbtt__buf_seek(&b, stbtt__buf_get8(&b)); // hdrsize

    // @TODO the name INDEX could list multiple fonts,
    // but we just use the first one.
    stbtt__cff_get_index(&b); // name INDEX
    topdictidx = stbtt__cff_get_index(&b);
    topdict = stbtt__cff_index_get(topdictidx, 0);
    stbtt__cff_get_index(&b); // string INDEX
    info->gsubrs = stbtt__cff_get_index(&b);

    stbtt__dict_get_ints(&topdict, 17, 1, &charstrings);
    stbtt__dict_get_ints(&topdict, 0x100 | 6, 1, &cstype);
    stbtt__dict_get_ints(&topdict, 0x100 | 36, 1, &fdarrayoff);
    stbtt__dict_get_ints(&topdict, 0x100 | 37, 1, &fdselectoff);
    info->subrs = stbtt__get_subrs(b, topdict);

    // we only support Type 2 charstrings
    if (cstype != 2) return 0;
    if (charstrings == 0) return 0;

    if (fdarrayoff) {
      // looks like a CID font
      if (!fdselectoff) return 0;
      stbtt__buf_seek(&b, fdarrayoff);
      info->fontdicts = stbtt__cff_get_index(&b);
      info->fdselect = stbtt__buf_range(&b, fdselectoff, b.size - fdselectoff);
    }

    stbtt__buf_seek(&b, charstrings);
    info->charstrings = stbtt__cff_get_index(&b);
  }

  t = stbtt__find_table(data, fontstart, "maxp");
  if (t)
    info->numGlyphs = ttUSHORT(data + t + 4);
  else
    info->numGlyphs = 0xffff;

  info->svg = -1;

  // find a cmap encoding table we understand *now* to avoid searching
  // later. (todo: could make this installable)
  // the same regardless of glyph.
  numTables = ttUSHORT(data + cmap + 2);
  info->index_map = 0;
  for (i = 0; i < numTables; ++i) {
    stbtt_uint32 encoding_record = cmap + 4 + 8 * i;
    // find an encoding we understand:
    switch (ttUSHORT(data + encoding_record)) {
    case STBTT_PLATFORM_ID_MICROSOFT:
      switch (ttUSHORT(data + encoding_record + 2)) {
      case STBTT_MS_EID_UNICODE_BMP:
      case STBTT_MS_EID_UNICODE_FULL:
        // MS/Unicode
        info->index_map = cmap + ttULONG(data + encoding_record + 4);
        break;
      }
      break;
    case STBTT_PLATFORM_ID_UNICODE:
      // Mac/iOS has these
      // all the encodingIDs are unicode, so we don't bother to check it
      info->index_map = cmap + ttULONG(data + encoding_record + 4);
      break;
    }
  }
  if (info->index_map == 0) return 0;

  info->indexToLocFormat = ttUSHORT(data + info->head + 50);
  return 1;
}

STBTT_DEF int stbtt_FindGlyphIndex(const stbtt_fontinfo *info, int unicode_codepoint) {
  stbtt_uint8 *data = info->data;
  stbtt_uint32 index_map = info->index_map;

  stbtt_uint16 format = ttUSHORT(data + index_map + 0);
  if (format == 0) { // apple byte encoding
    stbtt_int32 bytes = ttUSHORT(data + index_map + 2);
    if (unicode_codepoint < bytes - 6) return ttBYTE(data + index_map + 6 + unicode_codepoint);
    return 0;
  } else if (format == 6) {
    stbtt_uint32 first = ttUSHORT(data + index_map + 6);
    stbtt_uint32 count = ttUSHORT(data + index_map + 8);
    if ((stbtt_uint32)unicode_codepoint >= first && (stbtt_uint32)unicode_codepoint < first + count)
      return ttUSHORT(data + index_map + 10 + (unicode_codepoint - first) * 2);
    return 0;
  } else if (format == 2) {
    STBTT_assert(0); // @TODO: high-byte mapping for japanese/chinese/korean
    return 0;
  } else if (format == 4) { // standard mapping for windows fonts: binary search collection of ranges
    stbtt_uint16 segcount = ttUSHORT(data + index_map + 6) >> 1;
    stbtt_uint16 searchRange = ttUSHORT(data + index_map + 8) >> 1;
    stbtt_uint16 entrySelector = ttUSHORT(data + index_map + 10);
    stbtt_uint16 rangeShift = ttUSHORT(data + index_map + 12) >> 1;

    // do a binary search of the segments
    stbtt_uint32 endCount = index_map + 14;
    stbtt_uint32 search = endCount;

    if (unicode_codepoint > 0xffff) return 0;

    // they lie from endCount .. endCount + segCount
    // but searchRange is the nearest power of two, so...
    if (unicode_codepoint >= ttUSHORT(data + search + rangeShift * 2)) search += rangeShift * 2;

    // now decrement to bias correctly to find smallest
    search -= 2;
    while (entrySelector) {
      stbtt_uint16 end;
      searchRange >>= 1;
      end = ttUSHORT(data + search + searchRange * 2);
      if (unicode_codepoint > end) search += searchRange * 2;
      --entrySelector;
    }
    search += 2;

    {
      stbtt_uint16 offset, start, last;
      stbtt_uint16 item = (stbtt_uint16)((search - endCount) >> 1);

      start = ttUSHORT(data + index_map + 14 + segcount * 2 + 2 + 2 * item);
      last = ttUSHORT(data + endCount + 2 * item);
      if (unicode_codepoint < start || unicode_codepoint > last) return 0;

      offset = ttUSHORT(data + index_map + 14 + segcount * 6 + 2 + 2 * item);
      if (offset == 0) return (stbtt_uint16)(unicode_codepoint + ttSHORT(data + index_map + 14 + segcount * 4 + 2 + 2 * item));

      return ttUSHORT(data + offset + (unicode_codepoint - start) * 2 + index_map + 14 + segcount * 6 + 2 + 2 * item);
    }
  } else if (format == 12 || format == 13) {
    stbtt_uint32 ngroups = ttULONG(data + index_map + 12);
    stbtt_int32 low, high;
    low = 0;
    high = (stbtt_int32)ngroups;
    // Binary search the right group.
    while (low < high) {
      stbtt_int32 mid = low + ((high - low) >> 1); // rounds down, so low <= mid < high
      stbtt_uint32 start_char = ttULONG(data + index_map + 16 + mid * 12);
      stbtt_uint32 end_char = ttULONG(data + index_map + 16 + mid * 12 + 4);
      if ((stbtt_uint32)unicode_codepoint < start_char)
        high = mid;
      else if ((stbtt_uint32)unicode_codepoint > end_char)
        low = mid + 1;
      else {
        stbtt_uint32 start_glyph = ttULONG(data + index_map + 16 + mid * 12 + 8);
        if (format == 12)
          return start_glyph + unicode_codepoint - start_char;
        else // format == 13
          return start_glyph;
      }
    }
    return 0; // not found
  }
  // @TODO
  STBTT_assert(0);
  return 0;
}

#if 0
STBTT_DEF int stbtt_GetCodepointShape(const stbtt_fontinfo *info, int unicode_codepoint, stbtt_vertex **vertices)
{
   return stbtt_GetGlyphShape(info, stbtt_FindGlyphIndex(info, unicode_codepoint), vertices);
}
#endif

static void stbtt_setvertex(stbtt_vertex *v, stbtt_uint8 type, stbtt_int32 x, stbtt_int32 y, stbtt_int32 cx, stbtt_int32 cy) {
  v->type = type;
  v->x = (stbtt_int16)x;
  v->y = (stbtt_int16)y;
  v->cx = (stbtt_int16)cx;
  v->cy = (stbtt_int16)cy;
}

static int stbtt__GetGlyfOffset(const stbtt_fontinfo *info, int glyph_index) {
  int g1, g2;

  STBTT_assert(!info->cff.size);

  if (glyph_index >= info->numGlyphs) return -1; // glyph index out of range
  if (info->indexToLocFormat >= 2) return -1;    // unknown index->glyph map format

  if (info->indexToLocFormat == 0) {
    g1 = info->glyf + ttUSHORT(info->data + info->loca + glyph_index * 2) * 2;
    g2 = info->glyf + ttUSHORT(info->data + info->loca + glyph_index * 2 + 2) * 2;
  } else {
    g1 = info->glyf + ttULONG(info->data + info->loca + glyph_index * 4);
    g2 = info->glyf + ttULONG(info->data + info->loca + glyph_index * 4 + 4);
  }

  return g1 == g2 ? -1 : g1; // if length is 0, return -1
}

static int stbtt__GetGlyphInfoT2(const stbtt_fontinfo *info, int glyph_index, int *x0, int *y0, int *x1, int *y1);

STBTT_DEF int stbtt_GetGlyphBox(const stbtt_fontinfo *info, int glyph_index, int *x0, int *y0, int *x1, int *y1) {
  if (info->cff.size) {
    stbtt__GetGlyphInfoT2(info, glyph_index, x0, y0, x1, y1);
  } else {
    int g = stbtt__GetGlyfOffset(info, glyph_index);
    if (g < 0) return 0;

    if (x0) *x0 = ttSHORT(info->data + g + 2);
    if (y0) *y0 = ttSHORT(info->data + g + 4);
    if (x1) *x1 = ttSHORT(info->data + g + 6);
    if (y1) *y1 = ttSHORT(info->data + g + 8);
  }
  return 1;
}

STBTT_DEF int stbtt_IsGlyphEmpty(const stbtt_fontinfo *info, int glyph_index) {
  stbtt_int16 numberOfContours;
  int g;
  if (info->cff.size) return stbtt__GetGlyphInfoT2(info, glyph_index, NULL, NULL, NULL, NULL) == 0;
  g = stbtt__GetGlyfOffset(info, glyph_index);
  if (g < 0) return 1;
  numberOfContours = ttSHORT(info->data + g);
  return numberOfContours == 0;
}

static int stbtt__close_shape(
  stbtt_vertex *vertices,
  int num_vertices,
  int was_off,
  int start_off,
  stbtt_int32 sx,
  stbtt_int32 sy,
  stbtt_int32 scx,
  stbtt_int32 scy,
  stbtt_int32 cx,
  stbtt_int32 cy) {
  if (start_off) {
    if (was_off) stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, (cx + scx) >> 1, (cy + scy) >> 1, cx, cy);
    stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, sx, sy, scx, scy);
  } else {
    if (was_off)
      stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, sx, sy, cx, cy);
    else
      stbtt_setvertex(&vertices[num_vertices++], STBTT_vline, sx, sy, 0, 0);
  }
  return num_vertices;
}

static int stbtt__GetGlyphShapeTT(const stbtt_fontinfo *info, int glyph_index, stbtt_vertex **pvertices) {
  stbtt_int16 numberOfContours;
  stbtt_uint8 *endPtsOfContours;
  stbtt_uint8 *data = info->data;
  stbtt_vertex *vertices = 0;
  int num_vertices = 0;
  int g = stbtt__GetGlyfOffset(info, glyph_index);

  *pvertices = NULL;

  if (g < 0) return 0;

  numberOfContours = ttSHORT(data + g);

  if (numberOfContours > 0) {
    stbtt_uint8 flags = 0, flagcount;
    stbtt_int32 ins, i, j = 0, m, n, next_move, was_off = 0, off, start_off = 0;
    stbtt_int32 x, y, cx, cy, sx, sy, scx, scy;
    stbtt_uint8 *points;
    endPtsOfContours = (data + g + 10);
    ins = ttUSHORT(data + g + 10 + numberOfContours * 2);
    points = data + g + 10 + numberOfContours * 2 + 2 + ins;

    n = 1 + ttUSHORT(endPtsOfContours + numberOfContours * 2 - 2);

    m = n + 2 * numberOfContours; // a loose bound on how many vertices we might need
    vertices = (stbtt_vertex *)STBTT_malloc(m * sizeof(vertices[0]), info->userdata);
    if (vertices == 0) return 0;

    next_move = 0;
    flagcount = 0;

    // in first pass, we load uninterpreted data into the allocated array
    // above, shifted to the end of the array so we won't overwrite it when
    // we create our final data starting from the front

    off = m - n; // starting offset for uninterpreted data, regardless of how m ends up being calculated

    // first load flags

    for (i = 0; i < n; ++i) {
      if (flagcount == 0) {
        flags = *points++;
        if (flags & 8) flagcount = *points++;
      } else
        --flagcount;
      vertices[off + i].type = flags;
    }

    // now load x coordinates
    x = 0;
    for (i = 0; i < n; ++i) {
      flags = vertices[off + i].type;
      if (flags & 2) {
        stbtt_int16 dx = *points++;
        x += (flags & 16) ? dx : -dx; // ???
      } else {
        if (!(flags & 16)) {
          x = x + (stbtt_int16)(points[0] * 256 + points[1]);
          points += 2;
        }
      }
      vertices[off + i].x = (stbtt_int16)x;
    }

    // now load y coordinates
    y = 0;
    for (i = 0; i < n; ++i) {
      flags = vertices[off + i].type;
      if (flags & 4) {
        stbtt_int16 dy = *points++;
        y += (flags & 32) ? dy : -dy; // ???
      } else {
        if (!(flags & 32)) {
          y = y + (stbtt_int16)(points[0] * 256 + points[1]);
          points += 2;
        }
      }
      vertices[off + i].y = (stbtt_int16)y;
    }

    // now convert them to our format
    num_vertices = 0;
    sx = sy = cx = cy = scx = scy = 0;
    for (i = 0; i < n; ++i) {
      flags = vertices[off + i].type;
      x = (stbtt_int16)vertices[off + i].x;
      y = (stbtt_int16)vertices[off + i].y;

      if (next_move == i) {
        if (i != 0) num_vertices = stbtt__close_shape(vertices, num_vertices, was_off, start_off, sx, sy, scx, scy, cx, cy);

        // now start the new one
        start_off = !(flags & 1);
        if (start_off) {
          // if we start off with an off-curve point, then when we need to find a point on the curve
          // where we can start, and we need to save some state for when we wraparound.
          scx = x;
          scy = y;
          if (!(vertices[off + i + 1].type & 1)) {
            // next point is also a curve point, so interpolate an on-point curve
            sx = (x + (stbtt_int32)vertices[off + i + 1].x) >> 1;
            sy = (y + (stbtt_int32)vertices[off + i + 1].y) >> 1;
          } else {
            // otherwise just use the next point as our start point
            sx = (stbtt_int32)vertices[off + i + 1].x;
            sy = (stbtt_int32)vertices[off + i + 1].y;
            ++i; // we're using point i+1 as the starting point, so skip it
          }
        } else {
          sx = x;
          sy = y;
        }
        stbtt_setvertex(&vertices[num_vertices++], STBTT_vmove, sx, sy, 0, 0);
        was_off = 0;
        next_move = 1 + ttUSHORT(endPtsOfContours + j * 2);
        ++j;
      } else {
        if (!(flags & 1)) { // if it's a curve
          if (was_off)      // two off-curve control points in a row means interpolate an on-curve midpoint
            stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, (cx + x) >> 1, (cy + y) >> 1, cx, cy);
          cx = x;
          cy = y;
          was_off = 1;
        } else {
          if (was_off)
            stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, x, y, cx, cy);
          else
            stbtt_setvertex(&vertices[num_vertices++], STBTT_vline, x, y, 0, 0);
          was_off = 0;
        }
      }
    }
    num_vertices = stbtt__close_shape(vertices, num_vertices, was_off, start_off, sx, sy, scx, scy, cx, cy);
  } else if (numberOfContours < 0) {
    // Compound shapes.
    int more = 1;
    stbtt_uint8 *comp = data + g + 10;
    num_vertices = 0;
    vertices = 0;
    while (more) {
      stbtt_uint16 flags, gidx;
      int comp_num_verts = 0, i;
      stbtt_vertex *comp_verts = 0, *tmp = 0;
      float mtx[6] = {1, 0, 0, 1, 0, 0}, m, n;

      flags = ttSHORT(comp);
      comp += 2;
      gidx = ttSHORT(comp);
      comp += 2;

      if (flags & 2) {   // XY values
        if (flags & 1) { // shorts
          mtx[4] = ttSHORT(comp);
          comp += 2;
          mtx[5] = ttSHORT(comp);
          comp += 2;
        } else {
          mtx[4] = ttCHAR(comp);
          comp += 1;
          mtx[5] = ttCHAR(comp);
          comp += 1;
        }
      } else {
        // @TODO handle matching point
        STBTT_assert(0);
      }
      if (flags & (1 << 3)) { // WE_HAVE_A_SCALE
        mtx[0] = mtx[3] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[1] = mtx[2] = 0;
      } else if (flags & (1 << 6)) { // WE_HAVE_AN_X_AND_YSCALE
        mtx[0] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[1] = mtx[2] = 0;
        mtx[3] = ttSHORT(comp) / 16384.0f;
        comp += 2;
      } else if (flags & (1 << 7)) { // WE_HAVE_A_TWO_BY_TWO
        mtx[0] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[1] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[2] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[3] = ttSHORT(comp) / 16384.0f;
        comp += 2;
      }

      // Find transformation scales.
      m = (float)STBTT_sqrt(mtx[0] * mtx[0] + mtx[1] * mtx[1]);
      n = (float)STBTT_sqrt(mtx[2] * mtx[2] + mtx[3] * mtx[3]);

      // Get indexed glyph.
      comp_num_verts = stbtt_GetGlyphShape(info, gidx, &comp_verts);
      if (comp_num_verts > 0) {
        // Transform vertices.
        for (i = 0; i < comp_num_verts; ++i) {
          stbtt_vertex *v = &comp_verts[i];
          stbtt_vertex_type x, y;
          x = v->x;
          y = v->y;
          v->x = (stbtt_vertex_type)(m * (mtx[0] * x + mtx[2] * y + mtx[4]));
          v->y = (stbtt_vertex_type)(n * (mtx[1] * x + mtx[3] * y + mtx[5]));
          x = v->cx;
          y = v->cy;
          v->cx = (stbtt_vertex_type)(m * (mtx[0] * x + mtx[2] * y + mtx[4]));
          v->cy = (stbtt_vertex_type)(n * (mtx[1] * x + mtx[3] * y + mtx[5]));
        }
        // Append vertices.
        tmp = (stbtt_vertex *)STBTT_malloc((num_vertices + comp_num_verts) * sizeof(stbtt_vertex), info->userdata);
        if (!tmp) {
          if (vertices) STBTT_free(vertices, info->userdata);
          if (comp_verts) STBTT_free(comp_verts, info->userdata);
          return 0;
        }
        if (num_vertices > 0 && vertices) STBTT_memcpy(tmp, vertices, num_vertices * sizeof(stbtt_vertex));
        STBTT_memcpy(tmp + num_vertices, comp_verts, comp_num_verts * sizeof(stbtt_vertex));
        if (vertices) STBTT_free(vertices, info->userdata);
        vertices = tmp;
        STBTT_free(comp_verts, info->userdata);
        num_vertices += comp_num_verts;
      }
      // More components ?
      more = flags & (1 << 5);
    }
  } else {
    // numberOfCounters == 0, do nothing
  }

  *pvertices = vertices;
  return num_vertices;
}

typedef struct {
  int bounds;
  int started;
  float first_x, first_y;
  float x, y;
  stbtt_int32 min_x, max_x, min_y, max_y;

  stbtt_vertex *pvertices;
  int num_vertices;
} stbtt__csctx;

#define STBTT__CSCTX_INIT(bounds) \
  { bounds, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0 }

static void stbtt__track_vertex(stbtt__csctx *c, stbtt_int32 x, stbtt_int32 y) {
  if (x > c->max_x || !c->started) c->max_x = x;
  if (y > c->max_y || !c->started) c->max_y = y;
  if (x < c->min_x || !c->started) c->min_x = x;
  if (y < c->min_y || !c->started) c->min_y = y;
  c->started = 1;
}

static void stbtt__csctx_v(
  stbtt__csctx *c,
  stbtt_uint8 type,
  stbtt_int32 x,
  stbtt_int32 y,
  stbtt_int32 cx,
  stbtt_int32 cy,
  stbtt_int32 cx1,
  stbtt_int32 cy1) {
  if (c->bounds) {
    stbtt__track_vertex(c, x, y);
    if (type == STBTT_vcubic) {
      stbtt__track_vertex(c, cx, cy);
      stbtt__track_vertex(c, cx1, cy1);
    }
  } else {
    stbtt_setvertex(&c->pvertices[c->num_vertices], type, x, y, cx, cy);
    c->pvertices[c->num_vertices].cx1 = (stbtt_int16)cx1;
    c->pvertices[c->num_vertices].cy1 = (stbtt_int16)cy1;
  }
  c->num_vertices++;
}

static void stbtt__csctx_close_shape(stbtt__csctx *ctx) {
  if (ctx->first_x != ctx->x || ctx->first_y != ctx->y)
    stbtt__csctx_v(ctx, STBTT_vline, (int)ctx->first_x, (int)ctx->first_y, 0, 0, 0, 0);
}

static void stbtt__csctx_rmove_to(stbtt__csctx *ctx, float dx, float dy) {
  stbtt__csctx_close_shape(ctx);
  ctx->first_x = ctx->x = ctx->x + dx;
  ctx->first_y = ctx->y = ctx->y + dy;
  stbtt__csctx_v(ctx, STBTT_vmove, (int)ctx->x, (int)ctx->y, 0, 0, 0, 0);
}

static void stbtt__csctx_rline_to(stbtt__csctx *ctx, float dx, float dy) {
  ctx->x += dx;
  ctx->y += dy;
  stbtt__csctx_v(ctx, STBTT_vline, (int)ctx->x, (int)ctx->y, 0, 0, 0, 0);
}

static void stbtt__csctx_rccurve_to(stbtt__csctx *ctx, float dx1, float dy1, float dx2, float dy2, float dx3, float dy3) {
  float cx1 = ctx->x + dx1;
  float cy1 = ctx->y + dy1;
  float cx2 = cx1 + dx2;
  float cy2 = cy1 + dy2;
  ctx->x = cx2 + dx3;
  ctx->y = cy2 + dy3;
  stbtt__csctx_v(ctx, STBTT_vcubic, (int)ctx->x, (int)ctx->y, (int)cx1, (int)cy1, (int)cx2, (int)cy2);
}

static stbtt__buf stbtt__get_subr(stbtt__buf idx, int n) {
  int count = stbtt__cff_index_count(&idx);
  int bias = 107;
  if (count >= 33900)
    bias = 32768;
  else if (count >= 1240)
    bias = 1131;
  n += bias;
  if (n < 0 || n >= count) return stbtt__new_buf(NULL, 0);
  return stbtt__cff_index_get(idx, n);
}

static stbtt__buf stbtt__cid_get_glyph_subrs(const stbtt_fontinfo *info, int glyph_index) {
  stbtt__buf fdselect = info->fdselect;
  int nranges, start, end, v, fmt, fdselector = -1, i;

  stbtt__buf_seek(&fdselect, 0);
  fmt = stbtt__buf_get8(&fdselect);
  if (fmt == 0) {
    // untested
    stbtt__buf_skip(&fdselect, glyph_index);
    fdselector = stbtt__buf_get8(&fdselect);
  } else if (fmt == 3) {
    nranges = stbtt__buf_get16(&fdselect);
    start = stbtt__buf_get16(&fdselect);
    for (i = 0; i < nranges; i++) {
      v = stbtt__buf_get8(&fdselect);
      end = stbtt__buf_get16(&fdselect);
      if (glyph_index >= start && glyph_index < end) {
        fdselector = v;
        break;
      }
      start = end;
    }
  }
  if (fdselector == -1) stbtt__new_buf(NULL, 0);
  return stbtt__get_subrs(info->cff, stbtt__cff_index_get(info->fontdicts, fdselector));
}

static int stbtt__run_charstring(const stbtt_fontinfo *info, int glyph_index, stbtt__csctx *c) {
  int in_header = 1, maskbits = 0, subr_stack_height = 0, sp = 0, v, i, b0;
  int has_subrs = 0, clear_stack;
  float s[48];
  stbtt__buf subr_stack[10], subrs = info->subrs, b;
  float f;

#define STBTT__CSERR(s) (0)

  // this currently ignores the initial width value, which isn't needed if we have hmtx
  b = stbtt__cff_index_get(info->charstrings, glyph_index);
  while (b.cursor < b.size) {
    i = 0;
    clear_stack = 1;
    b0 = stbtt__buf_get8(&b);
    switch (b0) {
    // @TODO implement hinting
    case 0x13:                             // hintmask
    case 0x14:                             // cntrmask
      if (in_header) maskbits += (sp / 2); // implicit "vstem"
      in_header = 0;
      stbtt__buf_skip(&b, (maskbits + 7) / 8);
      break;

    case 0x01: // hstem
    case 0x03: // vstem
    case 0x12: // hstemhm
    case 0x17: // vstemhm
      maskbits += (sp / 2);
      break;

    case 0x15: // rmoveto
      in_header = 0;
      if (sp < 2) return STBTT__CSERR("rmoveto stack");
      stbtt__csctx_rmove_to(c, s[sp - 2], s[sp - 1]);
      break;
    case 0x04: // vmoveto
      in_header = 0;
      if (sp < 1) return STBTT__CSERR("vmoveto stack");
      stbtt__csctx_rmove_to(c, 0, s[sp - 1]);
      break;
    case 0x16: // hmoveto
      in_header = 0;
      if (sp < 1) return STBTT__CSERR("hmoveto stack");
      stbtt__csctx_rmove_to(c, s[sp - 1], 0);
      break;

    case 0x05: // rlineto
      if (sp < 2) return STBTT__CSERR("rlineto stack");
      for (; i + 1 < sp; i += 2) stbtt__csctx_rline_to(c, s[i], s[i + 1]);
      break;

      // hlineto/vlineto and vhcurveto/hvcurveto alternate horizontal and vertical
      // starting from a different place.

    case 0x07: // vlineto
      if (sp < 1) return STBTT__CSERR("vlineto stack");
      goto vlineto;
    case 0x06: // hlineto
      if (sp < 1) return STBTT__CSERR("hlineto stack");
      for (;;) {
        if (i >= sp) break;
        stbtt__csctx_rline_to(c, s[i], 0);
        i++;
      vlineto:
        if (i >= sp) break;
        stbtt__csctx_rline_to(c, 0, s[i]);
        i++;
      }
      break;

    case 0x1F: // hvcurveto
      if (sp < 4) return STBTT__CSERR("hvcurveto stack");
      goto hvcurveto;
    case 0x1E: // vhcurveto
      if (sp < 4) return STBTT__CSERR("vhcurveto stack");
      for (;;) {
        if (i + 3 >= sp) break;
        stbtt__csctx_rccurve_to(c, 0, s[i], s[i + 1], s[i + 2], s[i + 3], (sp - i == 5) ? s[i + 4] : 0.0f);
        i += 4;
      hvcurveto:
        if (i + 3 >= sp) break;
        stbtt__csctx_rccurve_to(c, s[i], 0, s[i + 1], s[i + 2], (sp - i == 5) ? s[i + 4] : 0.0f, s[i + 3]);
        i += 4;
      }
      break;

    case 0x08: // rrcurveto
      if (sp < 6) return STBTT__CSERR("rcurveline stack");
      for (; i + 5 < sp; i += 6) stbtt__csctx_rccurve_to(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4], s[i + 5]);
      break;

    case 0x18: // rcurveline
      if (sp < 8) return STBTT__CSERR("rcurveline stack");
      for (; i + 5 < sp - 2; i += 6) stbtt__csctx_rccurve_to(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4], s[i + 5]);
      if (i + 1 >= sp) return STBTT__CSERR("rcurveline stack");
      stbtt__csctx_rline_to(c, s[i], s[i + 1]);
      break;

    case 0x19: // rlinecurve
      if (sp < 8) return STBTT__CSERR("rlinecurve stack");
      for (; i + 1 < sp - 6; i += 2) stbtt__csctx_rline_to(c, s[i], s[i + 1]);
      if (i + 5 >= sp) return STBTT__CSERR("rlinecurve stack");
      stbtt__csctx_rccurve_to(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4], s[i + 5]);
      break;

    case 0x1A: // vvcurveto
    case 0x1B: // hhcurveto
      if (sp < 4) return STBTT__CSERR("(vv|hh)curveto stack");
      f = 0.0;
      if (sp & 1) {
        f = s[i];
        i++;
      }
      for (; i + 3 < sp; i += 4) {
        if (b0 == 0x1B)
          stbtt__csctx_rccurve_to(c, s[i], f, s[i + 1], s[i + 2], s[i + 3], 0.0);
        else
          stbtt__csctx_rccurve_to(c, f, s[i], s[i + 1], s[i + 2], 0.0, s[i + 3]);
        f = 0.0;
      }
      break;

    case 0x0A: // callsubr
      if (!has_subrs) {
        if (info->fdselect.size) subrs = stbtt__cid_get_glyph_subrs(info, glyph_index);
        has_subrs = 1;
      }
      // FALLTHROUGH
    case 0x1D: // callgsubr
      if (sp < 1) return STBTT__CSERR("call(g|)subr stack");
      v = (int)s[--sp];
      if (subr_stack_height >= 10) return STBTT__CSERR("recursion limit");
      subr_stack[subr_stack_height++] = b;
      b = stbtt__get_subr(b0 == 0x0A ? subrs : info->gsubrs, v);
      if (b.size == 0) return STBTT__CSERR("subr not found");
      b.cursor = 0;
      clear_stack = 0;
      break;

    case 0x0B: // return
      if (subr_stack_height <= 0) return STBTT__CSERR("return outside subr");
      b = subr_stack[--subr_stack_height];
      clear_stack = 0;
      break;

    case 0x0E: // endchar
      stbtt__csctx_close_shape(c);
      return 1;

    case 0x0C: { // two-byte escape
      float dx1, dx2, dx3, dx4, dx5, dx6, dy1, dy2, dy3, dy4, dy5, dy6;
      float dx, dy;
      int b1 = stbtt__buf_get8(&b);
      switch (b1) {
      // @TODO These "flex" implementations ignore the flex-depth and resolution,
      // and always draw beziers.
      case 0x22: // hflex
        if (sp < 7) return STBTT__CSERR("hflex stack");
        dx1 = s[0];
        dx2 = s[1];
        dy2 = s[2];
        dx3 = s[3];
        dx4 = s[4];
        dx5 = s[5];
        dx6 = s[6];
        stbtt__csctx_rccurve_to(c, dx1, 0, dx2, dy2, dx3, 0);
        stbtt__csctx_rccurve_to(c, dx4, 0, dx5, -dy2, dx6, 0);
        break;

      case 0x23: // flex
        if (sp < 13) return STBTT__CSERR("flex stack");
        dx1 = s[0];
        dy1 = s[1];
        dx2 = s[2];
        dy2 = s[3];
        dx3 = s[4];
        dy3 = s[5];
        dx4 = s[6];
        dy4 = s[7];
        dx5 = s[8];
        dy5 = s[9];
        dx6 = s[10];
        dy6 = s[11];
        // fd is s[12]
        stbtt__csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, dy3);
        stbtt__csctx_rccurve_to(c, dx4, dy4, dx5, dy5, dx6, dy6);
        break;

      case 0x24: // hflex1
        if (sp < 9) return STBTT__CSERR("hflex1 stack");
        dx1 = s[0];
        dy1 = s[1];
        dx2 = s[2];
        dy2 = s[3];
        dx3 = s[4];
        dx4 = s[5];
        dx5 = s[6];
        dy5 = s[7];
        dx6 = s[8];
        stbtt__csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, 0);
        stbtt__csctx_rccurve_to(c, dx4, 0, dx5, dy5, dx6, -(dy1 + dy2 + dy5));
        break;

      case 0x25: // flex1
        if (sp < 11) return STBTT__CSERR("flex1 stack");
        dx1 = s[0];
        dy1 = s[1];
        dx2 = s[2];
        dy2 = s[3];
        dx3 = s[4];
        dy3 = s[5];
        dx4 = s[6];
        dy4 = s[7];
        dx5 = s[8];
        dy5 = s[9];
        dx6 = dy6 = s[10];
        dx = dx1 + dx2 + dx3 + dx4 + dx5;
        dy = dy1 + dy2 + dy3 + dy4 + dy5;
        if (STBTT_fabs(dx) > STBTT_fabs(dy))
          dy6 = -dy;
        else
          dx6 = -dx;
        stbtt__csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, dy3);
        stbtt__csctx_rccurve_to(c, dx4, dy4, dx5, dy5, dx6, dy6);
        break;

      default: return STBTT__CSERR("unimplemented");
      }
    } break;

    default:
      if (b0 != 255 && b0 != 28 && b0 < 32) return STBTT__CSERR("reserved operator");

      // push immediate
      if (b0 == 255) {
        f = (float)(stbtt_int32)stbtt__buf_get32(&b) / 0x10000;
      } else {
        stbtt__buf_skip(&b, -1);
        f = (float)(stbtt_int16)stbtt__cff_int(&b);
      }
      if (sp >= 48) return STBTT__CSERR("push stack overflow");
      s[sp++] = f;
      clear_stack = 0;
      break;
    }
    if (clear_stack) sp = 0;
  }
  return STBTT__CSERR("no endchar");

#undef STBTT__CSERR
}

static int stbtt__GetGlyphShapeT2(const stbtt_fontinfo *info, int glyph_index, stbtt_vertex **pvertices) {
  // runs the charstring twice, once to count and once to output (to avoid realloc)
  stbtt__csctx count_ctx = STBTT__CSCTX_INIT(1);
  stbtt__csctx output_ctx = STBTT__CSCTX_INIT(0);
  if (stbtt__run_charstring(info, glyph_index, &count_ctx)) {
    *pvertices = (stbtt_vertex *)STBTT_malloc(count_ctx.num_vertices * sizeof(stbtt_vertex), info->userdata);
    output_ctx.pvertices = *pvertices;
    if (stbtt__run_charstring(info, glyph_index, &output_ctx)) {
      STBTT_assert(output_ctx.num_vertices == count_ctx.num_vertices);
      return output_ctx.num_vertices;
    }
  }
  *pvertices = NULL;
  return 0;
}

static int stbtt__GetGlyphInfoT2(const stbtt_fontinfo *info, int glyph_index, int *x0, int *y0, int *x1, int *y1) {
  stbtt__csctx c = STBTT__CSCTX_INIT(1);
  int r = stbtt__run_charstring(info, glyph_index, &c);
  if (x0) *x0 = r ? c.min_x : 0;
  if (y0) *y0 = r ? c.min_y : 0;
  if (x1) *x1 = r ? c.max_x : 0;
  if (y1) *y1 = r ? c.max_y : 0;
  return r ? c.num_vertices : 0;
}

STBTT_DEF int stbtt_GetGlyphShape(const stbtt_fontinfo *info, int glyph_index, stbtt_vertex **pvertices) {
  if (!info->cff.size)
    return stbtt__GetGlyphShapeTT(info, glyph_index, pvertices);
  else
    return stbtt__GetGlyphShapeT2(info, glyph_index, pvertices);
}

STBTT_DEF void stbtt_GetGlyphHMetrics(const stbtt_fontinfo *info, int glyph_index, int *advanceWidth, int *leftSideBearing) {
  stbtt_uint16 numOfLongHorMetrics = ttUSHORT(info->data + info->hhea + 34);
  if (glyph_index < numOfLongHorMetrics) {
    if (advanceWidth) *advanceWidth = ttSHORT(info->data + info->hmtx + 4 * glyph_index);
    if (leftSideBearing) *leftSideBearing = ttSHORT(info->data + info->hmtx + 4 * glyph_index + 2);
  } else {
    if (advanceWidth) *advanceWidth = ttSHORT(info->data + info->hmtx + 4 * (numOfLongHorMetrics - 1));
    if (leftSideBearing)
      *leftSideBearing = ttSHORT(info->data + info->hmtx + 4 * numOfLongHorMetrics + 2 * (glyph_index - numOfLongHorMetrics));
  }
}

STBTT_DEF int stbtt_GetKerningTableLength(const stbtt_fontinfo *info) {
  stbtt_uint8 *data = info->data + info->kern;

  // we only look at the first table. it must be 'horizontal' and format 0.
  if (!info->kern) return 0;
  if (ttUSHORT(data + 2) < 1) // number of tables, need at least 1
    return 0;
  if (ttUSHORT(data + 8) != 1) // horizontal flag must be set in format
    return 0;

  return ttUSHORT(data + 10);
}

STBTT_DEF int stbtt_GetKerningTable(const stbtt_fontinfo *info, stbtt_kerningentry *table, int table_length) {
  stbtt_uint8 *data = info->data + info->kern;
  int k, length;

  // we only look at the first table. it must be 'horizontal' and format 0.
  if (!info->kern) return 0;
  if (ttUSHORT(data + 2) < 1) // number of tables, need at least 1
    return 0;
  if (ttUSHORT(data + 8) != 1) // horizontal flag must be set in format
    return 0;

  length = ttUSHORT(data + 10);
  if (table_length < length) length = table_length;

  for (k = 0; k < length; k++) {
    table[k].glyph1 = ttUSHORT(data + 18 + (k * 6));
    table[k].glyph2 = ttUSHORT(data + 20 + (k * 6));
    table[k].advance = ttSHORT(data + 22 + (k * 6));
  }

  return length;
}

static int stbtt__GetGlyphKernInfoAdvance(const stbtt_fontinfo *info, int glyph1, int glyph2) {
  stbtt_uint8 *data = info->data + info->kern;
  stbtt_uint32 needle, straw;
  int l, r, m;

  // we only look at the first table. it must be 'horizontal' and format 0.
  if (!info->kern) return 0;
  if (ttUSHORT(data + 2) < 1) // number of tables, need at least 1
    return 0;
  if (ttUSHORT(data + 8) != 1) // horizontal flag must be set in format
    return 0;

  l = 0;
  r = ttUSHORT(data + 10) - 1;
  needle = glyph1 << 16 | glyph2;
  while (l <= r) {
    m = (l + r) >> 1;
    straw = ttULONG(data + 18 + (m * 6)); // note: unaligned read
    if (needle < straw)
      r = m - 1;
    else if (needle > straw)
      l = m + 1;
    else
      return ttSHORT(data + 22 + (m * 6));
  }
  return 0;
}

static stbtt_int32 stbtt__GetCoverageIndex(stbtt_uint8 *coverageTable, int glyph) {
  stbtt_uint16 coverageFormat = ttUSHORT(coverageTable);
  switch (coverageFormat) {
  case 1: {
    stbtt_uint16 glyphCount = ttUSHORT(coverageTable + 2);

    // Binary search.
    stbtt_int32 l = 0, r = glyphCount - 1, m;
    int straw, needle = glyph;
    while (l <= r) {
      stbtt_uint8 *glyphArray = coverageTable + 4;
      stbtt_uint16 glyphID;
      m = (l + r) >> 1;
      glyphID = ttUSHORT(glyphArray + 2 * m);
      straw = glyphID;
      if (needle < straw)
        r = m - 1;
      else if (needle > straw)
        l = m + 1;
      else {
        return m;
      }
    }
    break;
  }

  case 2: {
    stbtt_uint16 rangeCount = ttUSHORT(coverageTable + 2);
    stbtt_uint8 *rangeArray = coverageTable + 4;

    // Binary search.
    stbtt_int32 l = 0, r = rangeCount - 1, m;
    int strawStart, strawEnd, needle = glyph;
    while (l <= r) {
      stbtt_uint8 *rangeRecord;
      m = (l + r) >> 1;
      rangeRecord = rangeArray + 6 * m;
      strawStart = ttUSHORT(rangeRecord);
      strawEnd = ttUSHORT(rangeRecord + 2);
      if (needle < strawStart)
        r = m - 1;
      else if (needle > strawEnd)
        l = m + 1;
      else {
        stbtt_uint16 startCoverageIndex = ttUSHORT(rangeRecord + 4);
        return startCoverageIndex + glyph - strawStart;
      }
    }
    break;
  }

  default: return -1; // unsupported
  }

  return -1;
}

static stbtt_int32 stbtt__GetGlyphClass(stbtt_uint8 *classDefTable, int glyph) {
  stbtt_uint16 classDefFormat = ttUSHORT(classDefTable);
  switch (classDefFormat) {
  case 1: {
    stbtt_uint16 startGlyphID = ttUSHORT(classDefTable + 2);
    stbtt_uint16 glyphCount = ttUSHORT(classDefTable + 4);
    stbtt_uint8 *classDef1ValueArray = classDefTable + 6;

    if (glyph >= startGlyphID && glyph < startGlyphID + glyphCount)
      return (stbtt_int32)ttUSHORT(classDef1ValueArray + 2 * (glyph - startGlyphID));
    break;
  }

  case 2: {
    stbtt_uint16 classRangeCount = ttUSHORT(classDefTable + 2);
    stbtt_uint8 *classRangeRecords = classDefTable + 4;

    // Binary search.
    stbtt_int32 l = 0, r = classRangeCount - 1, m;
    int strawStart, strawEnd, needle = glyph;
    while (l <= r) {
      stbtt_uint8 *classRangeRecord;
      m = (l + r) >> 1;
      classRangeRecord = classRangeRecords + 6 * m;
      strawStart = ttUSHORT(classRangeRecord);
      strawEnd = ttUSHORT(classRangeRecord + 2);
      if (needle < strawStart)
        r = m - 1;
      else if (needle > strawEnd)
        l = m + 1;
      else
        return (stbtt_int32)ttUSHORT(classRangeRecord + 4);
    }
    break;
  }

  default: return -1; // Unsupported definition type, return an error.
  }

  // "All glyphs not assigned to a class fall into class 0". (OpenType spec)
  return 0;
}

// Define to STBTT_assert(x) if you want to break on unimplemented formats.
#define STBTT_GPOS_TODO_assert(x)

static stbtt_int32 stbtt__GetGlyphGPOSInfoAdvance(const stbtt_fontinfo *info, int glyph1, int glyph2) {
  stbtt_uint16 lookupListOffset;
  stbtt_uint8 *lookupList;
  stbtt_uint16 lookupCount;
  stbtt_uint8 *data;
  stbtt_int32 i, sti;

  if (!info->gpos) return 0;

  data = info->data + info->gpos;

  if (ttUSHORT(data + 0) != 1) return 0; // Major version 1
  if (ttUSHORT(data + 2) != 0) return 0; // Minor version 0

  lookupListOffset = ttUSHORT(data + 8);
  lookupList = data + lookupListOffset;
  lookupCount = ttUSHORT(lookupList);

  for (i = 0; i < lookupCount; ++i) {
    stbtt_uint16 lookupOffset = ttUSHORT(lookupList + 2 + 2 * i);
    stbtt_uint8 *lookupTable = lookupList + lookupOffset;

    stbtt_uint16 lookupType = ttUSHORT(lookupTable);
    stbtt_uint16 subTableCount = ttUSHORT(lookupTable + 4);
    stbtt_uint8 *subTableOffsets = lookupTable + 6;
    if (lookupType != 2) // Pair Adjustment Positioning Subtable
      continue;

    for (sti = 0; sti < subTableCount; sti++) {
      stbtt_uint16 subtableOffset = ttUSHORT(subTableOffsets + 2 * sti);
      stbtt_uint8 *table = lookupTable + subtableOffset;
      stbtt_uint16 posFormat = ttUSHORT(table);
      stbtt_uint16 coverageOffset = ttUSHORT(table + 2);
      stbtt_int32 coverageIndex = stbtt__GetCoverageIndex(table + coverageOffset, glyph1);
      if (coverageIndex == -1) continue;

      switch (posFormat) {
      case 1: {
        stbtt_int32 l, r, m;
        int straw, needle;
        stbtt_uint16 valueFormat1 = ttUSHORT(table + 4);
        stbtt_uint16 valueFormat2 = ttUSHORT(table + 6);
        if (valueFormat1 == 4 && valueFormat2 == 0) { // Support more formats?
          stbtt_int32 valueRecordPairSizeInBytes = 2;
          stbtt_uint16 pairSetCount = ttUSHORT(table + 8);
          stbtt_uint16 pairPosOffset = ttUSHORT(table + 10 + 2 * coverageIndex);
          stbtt_uint8 *pairValueTable = table + pairPosOffset;
          stbtt_uint16 pairValueCount = ttUSHORT(pairValueTable);
          stbtt_uint8 *pairValueArray = pairValueTable + 2;

          if (coverageIndex >= pairSetCount) return 0;

          needle = glyph2;
          r = pairValueCount - 1;
          l = 0;

          // Binary search.
          while (l <= r) {
            stbtt_uint16 secondGlyph;
            stbtt_uint8 *pairValue;
            m = (l + r) >> 1;
            pairValue = pairValueArray + (2 + valueRecordPairSizeInBytes) * m;
            secondGlyph = ttUSHORT(pairValue);
            straw = secondGlyph;
            if (needle < straw)
              r = m - 1;
            else if (needle > straw)
              l = m + 1;
            else {
              stbtt_int16 xAdvance = ttSHORT(pairValue + 2);
              return xAdvance;
            }
          }
        } else
          return 0;
        break;
      }

      case 2: {
        stbtt_uint16 valueFormat1 = ttUSHORT(table + 4);
        stbtt_uint16 valueFormat2 = ttUSHORT(table + 6);
        if (valueFormat1 == 4 && valueFormat2 == 0) { // Support more formats?
          stbtt_uint16 classDef1Offset = ttUSHORT(table + 8);
          stbtt_uint16 classDef2Offset = ttUSHORT(table + 10);
          int glyph1class = stbtt__GetGlyphClass(table + classDef1Offset, glyph1);
          int glyph2class = stbtt__GetGlyphClass(table + classDef2Offset, glyph2);

          stbtt_uint16 class1Count = ttUSHORT(table + 12);
          stbtt_uint16 class2Count = ttUSHORT(table + 14);
          stbtt_uint8 *class1Records, *class2Records;
          stbtt_int16 xAdvance;

          if (glyph1class < 0 || glyph1class >= class1Count) return 0; // malformed
          if (glyph2class < 0 || glyph2class >= class2Count) return 0; // malformed

          class1Records = table + 16;
          class2Records = class1Records + 2 * (glyph1class * class2Count);
          xAdvance = ttSHORT(class2Records + 2 * glyph2class);
          return xAdvance;
        } else
          return 0;
        break;
      }

      default: return 0; // Unsupported position format
      }
    }
  }

  return 0;
}

STBTT_DEF int stbtt_GetGlyphKernAdvance(const stbtt_fontinfo *info, int g1, int g2) {
  int xAdvance = 0;

  if (info->gpos)
    xAdvance += stbtt__GetGlyphGPOSInfoAdvance(info, g1, g2);
  else if (info->kern)
    xAdvance += stbtt__GetGlyphKernInfoAdvance(info, g1, g2);

  return xAdvance;
}

#if 0
STBTT_DEF int  stbtt_GetCodepointKernAdvance(const stbtt_fontinfo *info, int ch1, int ch2)
{
   if (!info->kern && !info->gpos) // if no kerning table, don't waste time looking up both codepoint->glyphs
      return 0;
   return stbtt_GetGlyphKernAdvance(info, stbtt_FindGlyphIndex(info,ch1), stbtt_FindGlyphIndex(info,ch2));
}

STBTT_DEF void stbtt_GetCodepointHMetrics(const stbtt_fontinfo *info, int codepoint, int *advanceWidth, int *leftSideBearing)
{
   stbtt_GetGlyphHMetrics(info, stbtt_FindGlyphIndex(info,codepoint), advanceWidth, leftSideBearing);
}
#endif

STBTT_DEF void stbtt_GetFontVMetrics(const stbtt_fontinfo *info, int *ascent, int *descent, int *lineGap) {
  if (ascent) *ascent = ttSHORT(info->data + info->hhea + 4);
  if (descent) *descent = ttSHORT(info->data + info->hhea + 6);
  if (lineGap) *lineGap = ttSHORT(info->data + info->hhea + 8);
}

#if 0
STBTT_DEF int  stbtt_GetFontVMetricsOS2(const stbtt_fontinfo *info, int *typoAscent, int *typoDescent, int *typoLineGap)
{
   int tab = stbtt__find_table(info->data, info->fontstart, "OS/2");
   if (!tab)
      return 0;
   if (typoAscent ) *typoAscent  = ttSHORT(info->data+tab + 68);
   if (typoDescent) *typoDescent = ttSHORT(info->data+tab + 70);
   if (typoLineGap) *typoLineGap = ttSHORT(info->data+tab + 72);
   return 1;
}

STBTT_DEF void stbtt_GetFontBoundingBox(const stbtt_fontinfo *info, int *x0, int *y0, int *x1, int *y1)
{
   *x0 = ttSHORT(info->data + info->head + 36);
   *y0 = ttSHORT(info->data + info->head + 38);
   *x1 = ttSHORT(info->data + info->head + 40);
   *y1 = ttSHORT(info->data + info->head + 42);
}
#endif

STBTT_DEF float stbtt_ScaleForMappingEmToPixels(const stbtt_fontinfo *info, float pixels) {
  int unitsPerEm = ttUSHORT(info->data + info->head + 18);
  return pixels / unitsPerEm;
}

#if 0
STBTT_DEF void stbtt_FreeShape(const stbtt_fontinfo *info, stbtt_vertex *v)
{
   STBTT_free(v, info->userdata);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// antialiasing software rasterizer
//

STBTT_DEF void stbtt_GetGlyphBitmapBoxSubpixel(
  const stbtt_fontinfo *font,
  int glyph,
  float scale_x,
  float scale_y,
  float shift_x,
  float shift_y,
  int *ix0,
  int *iy0,
  int *ix1,
  int *iy1) {
  int x0 = 0, y0 = 0, x1, y1; // =0 suppresses compiler warning
  if (!stbtt_GetGlyphBox(font, glyph, &x0, &y0, &x1, &y1)) {
    // e.g. space character
    if (ix0) *ix0 = 0;
    if (iy0) *iy0 = 0;
    if (ix1) *ix1 = 0;
    if (iy1) *iy1 = 0;
  } else {
    // move to integral bboxes (treating pixels as little squares, what pixels get touched)?
    if (ix0) *ix0 = STBTT_ifloor(x0 * scale_x + shift_x);
    if (iy0) *iy0 = STBTT_ifloor(-y1 * scale_y + shift_y);
    if (ix1) *ix1 = STBTT_iceil(x1 * scale_x + shift_x);
    if (iy1) *iy1 = STBTT_iceil(-y0 * scale_y + shift_y);
  }
}

//////////////////////////////////////////////////////////////////////////////
//
// sdf computation
//

#define STBTT_min(a, b) ((a) < (b) ? (a) : (b))
#define STBTT_max(a, b) ((a) < (b) ? (b) : (a))

static int stbtt__ray_intersect_bezier(float orig[2], float ray[2], float q0[2], float q1[2], float q2[2], float hits[2][2]) {
  float q0perp = q0[1] * ray[0] - q0[0] * ray[1];
  float q1perp = q1[1] * ray[0] - q1[0] * ray[1];
  float q2perp = q2[1] * ray[0] - q2[0] * ray[1];
  float roperp = orig[1] * ray[0] - orig[0] * ray[1];

  float a = q0perp - 2 * q1perp + q2perp;
  float b = q1perp - q0perp;
  float c = q0perp - roperp;

  float s0 = 0., s1 = 0.;
  int num_s = 0;

  if (a != 0.0) {
    float discr = b * b - a * c;
    if (discr > 0.0) {
      float rcpna = -1 / a;
      float d = (float)STBTT_sqrt(discr);
      s0 = (b + d) * rcpna;
      s1 = (b - d) * rcpna;
      if (s0 >= 0.0 && s0 <= 1.0) num_s = 1;
      if (d > 0.0 && s1 >= 0.0 && s1 <= 1.0) {
        if (num_s == 0) s0 = s1;
        ++num_s;
      }
    }
  } else {
    // 2*b*s + c = 0
    // s = -c / (2*b)
    s0 = c / (-2 * b);
    if (s0 >= 0.0 && s0 <= 1.0) num_s = 1;
  }

  if (num_s == 0)
    return 0;
  else {
    float rcp_len2 = 1 / (ray[0] * ray[0] + ray[1] * ray[1]);
    float rayn_x = ray[0] * rcp_len2, rayn_y = ray[1] * rcp_len2;

    float q0d = q0[0] * rayn_x + q0[1] * rayn_y;
    float q1d = q1[0] * rayn_x + q1[1] * rayn_y;
    float q2d = q2[0] * rayn_x + q2[1] * rayn_y;
    float rod = orig[0] * rayn_x + orig[1] * rayn_y;

    float q10d = q1d - q0d;
    float q20d = q2d - q0d;
    float q0rd = q0d - rod;

    hits[0][0] = q0rd + s0 * (2.0f - 2.0f * s0) * q10d + s0 * s0 * q20d;
    hits[0][1] = a * s0 + b;

    if (num_s > 1) {
      hits[1][0] = q0rd + s1 * (2.0f - 2.0f * s1) * q10d + s1 * s1 * q20d;
      hits[1][1] = a * s1 + b;
      return 2;
    } else {
      return 1;
    }
  }
}

static int equal(float *a, float *b) { return (a[0] == b[0] && a[1] == b[1]); }

static int stbtt__compute_crossings_x(float x, float y, int nverts, stbtt_vertex *verts) {
  int i;
  float orig[2], ray[2] = {1, 0};
  float y_frac;
  int winding = 0;

  // make sure y never passes through a vertex of the shape
  y_frac = (float)STBTT_fmod(y, 1.0f);
  if (y_frac < 0.01f)
    y += 0.01f;
  else if (y_frac > 0.99f)
    y -= 0.01f;

  orig[0] = x;
  orig[1] = y;

  // test a ray from (-infinity,y) to (x,y)
  for (i = 0; i < nverts; ++i) {
    if (verts[i].type == STBTT_vline) {
      int x0 = (int)verts[i - 1].x, y0 = (int)verts[i - 1].y;
      int x1 = (int)verts[i].x, y1 = (int)verts[i].y;
      if (y > STBTT_min(y0, y1) && y < STBTT_max(y0, y1) && x > STBTT_min(x0, x1)) {
        float x_inter = (y - y0) / (y1 - y0) * (x1 - x0) + x0;
        if (x_inter < x) winding += (y0 < y1) ? 1 : -1;
      }
    }
    if (verts[i].type == STBTT_vcurve) {
      int x0 = (int)verts[i - 1].x, y0 = (int)verts[i - 1].y;
      int x1 = (int)verts[i].cx, y1 = (int)verts[i].cy;
      int x2 = (int)verts[i].x, y2 = (int)verts[i].y;
      int ax = STBTT_min(x0, STBTT_min(x1, x2)), ay = STBTT_min(y0, STBTT_min(y1, y2));
      int by = STBTT_max(y0, STBTT_max(y1, y2));
      if (y > ay && y < by && x > ax) {
        float q0[2], q1[2], q2[2];
        float hits[2][2];
        q0[0] = (float)x0;
        q0[1] = (float)y0;
        q1[0] = (float)x1;
        q1[1] = (float)y1;
        q2[0] = (float)x2;
        q2[1] = (float)y2;
        if (equal(q0, q1) || equal(q1, q2)) {
          x0 = (int)verts[i - 1].x;
          y0 = (int)verts[i - 1].y;
          x1 = (int)verts[i].x;
          y1 = (int)verts[i].y;
          if (y > STBTT_min(y0, y1) && y < STBTT_max(y0, y1) && x > STBTT_min(x0, x1)) {
            float x_inter = (y - y0) / (y1 - y0) * (x1 - x0) + x0;
            if (x_inter < x) winding += (y0 < y1) ? 1 : -1;
          }
        } else {
          int num_hits = stbtt__ray_intersect_bezier(orig, ray, q0, q1, q2, hits);
          if (num_hits >= 1)
            if (hits[0][0] < 0) winding += (hits[0][1] < 0 ? -1 : 1);
          if (num_hits >= 2)
            if (hits[1][0] < 0) winding += (hits[1][1] < 0 ? -1 : 1);
        }
      }
    }
  }
  return winding;
}

static float stbtt__cuberoot(float x) {
  if (x < 0)
    return -(float)STBTT_pow(-x, 1.0f / 3.0f);
  else
    return (float)STBTT_pow(x, 1.0f / 3.0f);
}

// x^3 + a*x^2 + b*x + c = 0
static int stbtt__solve_cubic(float a, float b, float c, float *r) {
  float s = -a / 3;
  float p = b - a * a / 3;
  float q = a * (2 * a * a - 9 * b) / 27 + c;
  float p3 = p * p * p;
  float d = q * q + 4 * p3 / 27;
  if (d >= 0) {
    float z = (float)STBTT_sqrt(d);
    float u = (-q + z) / 2;
    float v = (-q - z) / 2;
    u = stbtt__cuberoot(u);
    v = stbtt__cuberoot(v);
    r[0] = s + u + v;
    return 1;
  } else {
    float u = (float)STBTT_sqrt(-p / 3);
    float v = (float)STBTT_acos(-STBTT_sqrt(-27 / p3) * q / 2) / 3; // p3 must be negative, since d is negative
    float m = (float)STBTT_cos(v);
    float n = (float)STBTT_cos(v - 3.141592 / 2) * 1.732050808f;
    r[0] = s + u * 2 * m;
    r[1] = s - u * (m + n);
    r[2] = s - u * (m - n);

    // STBTT_assert( STBTT_fabs(((r[0]+a)*r[0]+b)*r[0]+c) < 0.05f);  // these asserts may not be safe at all scales, though
    // they're in bezier t parameter units so maybe? STBTT_assert( STBTT_fabs(((r[1]+a)*r[1]+b)*r[1]+c) < 0.05f); STBTT_assert(
    // STBTT_fabs(((r[2]+a)*r[2]+b)*r[2]+c) < 0.05f);
    return 3;
  }
}

STBTT_DEF unsigned char *stbtt_GetGlyphSDF(
  const stbtt_fontinfo *info,
  float scale,
  int glyph,
  int padding,
  unsigned char onedge_value,
  float pixel_dist_scale,
  int *width,
  int *height,
  int *xoff,
  int *yoff) {
  float scale_x = scale, scale_y = scale;
  int ix0, iy0, ix1, iy1;
  int w, h;
  unsigned char *data;

  if (scale == 0) return NULL;

  stbtt_GetGlyphBitmapBoxSubpixel(info, glyph, scale, scale, 0.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);

  // if empty, return NULL
  if (ix0 == ix1 || iy0 == iy1) return NULL;

  ix0 -= padding;
  iy0 -= padding;
  ix1 += padding;
  iy1 += padding;

  w = (ix1 - ix0);
  h = (iy1 - iy0);

  if (width) *width = w;
  if (height) *height = h;
  if (xoff) *xoff = ix0;
  if (yoff) *yoff = iy0;

  // invert for y-downwards bitmaps
  scale_y = -scale_y;

  {
    int x, y, i, j;
    float *precompute;
    stbtt_vertex *verts;
    int num_verts = stbtt_GetGlyphShape(info, glyph, &verts);
    data = (unsigned char *)STBTT_malloc(w * h, info->userdata);
    precompute = (float *)STBTT_malloc(num_verts * sizeof(float), info->userdata);

    for (i = 0, j = num_verts - 1; i < num_verts; j = i++) {
      if (verts[i].type == STBTT_vline) {
        float x0 = verts[i].x * scale_x, y0 = verts[i].y * scale_y;
        float x1 = verts[j].x * scale_x, y1 = verts[j].y * scale_y;
        float dist = (float)STBTT_sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
        precompute[i] = (dist == 0) ? 0.0f : 1.0f / dist;
      } else if (verts[i].type == STBTT_vcurve) {
        float x2 = verts[j].x * scale_x, y2 = verts[j].y * scale_y;
        float x1 = verts[i].cx * scale_x, y1 = verts[i].cy * scale_y;
        float x0 = verts[i].x * scale_x, y0 = verts[i].y * scale_y;
        float bx = x0 - 2 * x1 + x2, by = y0 - 2 * y1 + y2;
        float len2 = bx * bx + by * by;
        if (len2 != 0.0f)
          precompute[i] = 1.0f / (bx * bx + by * by);
        else
          precompute[i] = 0.0f;
      } else
        precompute[i] = 0.0f;
    }

    for (y = iy0; y < iy1; ++y) {
      for (x = ix0; x < ix1; ++x) {
        float val;
        float min_dist = 999999.0f;
        float sx = (float)x + 0.5f;
        float sy = (float)y + 0.5f;
        float x_gspace = (sx / scale_x);
        float y_gspace = (sy / scale_y);

        int winding = stbtt__compute_crossings_x(
          x_gspace, y_gspace, num_verts,
          verts); // @OPTIMIZE: this could just be a rasterization, but needs to be line vs. non-tesselated curves so a new path

        for (i = 0; i < num_verts; ++i) {
          float x0 = verts[i].x * scale_x, y0 = verts[i].y * scale_y;

          if (verts[i].type == STBTT_vline && precompute[i] != 0.0f) {
            float x1 = verts[i - 1].x * scale_x, y1 = verts[i - 1].y * scale_y;

            float dist, dist2 = (x0 - sx) * (x0 - sx) + (y0 - sy) * (y0 - sy);
            if (dist2 < min_dist * min_dist) min_dist = (float)STBTT_sqrt(dist2);

            // coarse culling against bbox
            // if (sx > STBTT_min(x0,x1)-min_dist && sx < STBTT_max(x0,x1)+min_dist &&
            //    sy > STBTT_min(y0,y1)-min_dist && sy < STBTT_max(y0,y1)+min_dist)
            dist = (float)STBTT_fabs((x1 - x0) * (y0 - sy) - (y1 - y0) * (x0 - sx)) * precompute[i];
            STBTT_assert(i != 0);
            if (dist < min_dist) {
              // check position along line
              // x' = x0 + t*(x1-x0), y' = y0 + t*(y1-y0)
              // minimize (x'-sx)*(x'-sx)+(y'-sy)*(y'-sy)
              float dx = x1 - x0, dy = y1 - y0;
              float px = x0 - sx, py = y0 - sy;
              // minimize (px+t*dx)^2 + (py+t*dy)^2 = px*px + 2*px*dx*t + t^2*dx*dx + py*py + 2*py*dy*t + t^2*dy*dy
              // derivative: 2*px*dx + 2*py*dy + (2*dx*dx+2*dy*dy)*t, set to 0 and solve
              float t = -(px * dx + py * dy) / (dx * dx + dy * dy);
              if (t >= 0.0f && t <= 1.0f) min_dist = dist;
            }
          } else if (verts[i].type == STBTT_vcurve) {
            float x2 = verts[i - 1].x * scale_x, y2 = verts[i - 1].y * scale_y;
            float x1 = verts[i].cx * scale_x, y1 = verts[i].cy * scale_y;
            float box_x0 = STBTT_min(STBTT_min(x0, x1), x2);
            float box_y0 = STBTT_min(STBTT_min(y0, y1), y2);
            float box_x1 = STBTT_max(STBTT_max(x0, x1), x2);
            float box_y1 = STBTT_max(STBTT_max(y0, y1), y2);
            // coarse culling against bbox to avoid computing cubic unnecessarily
            if (sx > box_x0 - min_dist && sx < box_x1 + min_dist && sy > box_y0 - min_dist && sy < box_y1 + min_dist) {
              int num = 0;
              float ax = x1 - x0, ay = y1 - y0;
              float bx = x0 - 2 * x1 + x2, by = y0 - 2 * y1 + y2;
              float mx = x0 - sx, my = y0 - sy;
              float res[3] = {0.f, 0.f, 0.f};
              float px, py, t, it, dist2;
              float a_inv = precompute[i];
              if (a_inv == 0.0) { // if a_inv is 0, it's 2nd degree so use quadratic formula
                float a = 3 * (ax * bx + ay * by);
                float b = 2 * (ax * ax + ay * ay) + (mx * bx + my * by);
                float c = mx * ax + my * ay;
                if (a == 0.0) { // if a is 0, it's linear
                  if (b != 0.0) {
                    res[num++] = -c / b;
                  }
                } else {
                  float discriminant = b * b - 4 * a * c;
                  if (discriminant < 0)
                    num = 0;
                  else {
                    float root = (float)STBTT_sqrt(discriminant);
                    res[0] = (-b - root) / (2 * a);
                    res[1] = (-b + root) / (2 * a);
                    num = 2; // don't bother distinguishing 1-solution case, as code below will still work
                  }
                }
              } else {
                float b = 3 * (ax * bx + ay * by) * a_inv; // could precompute this as it doesn't depend on sample point
                float c = (2 * (ax * ax + ay * ay) + (mx * bx + my * by)) * a_inv;
                float d = (mx * ax + my * ay) * a_inv;
                num = stbtt__solve_cubic(b, c, d, res);
              }
              dist2 = (x0 - sx) * (x0 - sx) + (y0 - sy) * (y0 - sy);
              if (dist2 < min_dist * min_dist) min_dist = (float)STBTT_sqrt(dist2);

              if (num >= 1 && res[0] >= 0.0f && res[0] <= 1.0f) {
                t = res[0], it = 1.0f - t;
                px = it * it * x0 + 2 * t * it * x1 + t * t * x2;
                py = it * it * y0 + 2 * t * it * y1 + t * t * y2;
                dist2 = (px - sx) * (px - sx) + (py - sy) * (py - sy);
                if (dist2 < min_dist * min_dist) min_dist = (float)STBTT_sqrt(dist2);
              }
              if (num >= 2 && res[1] >= 0.0f && res[1] <= 1.0f) {
                t = res[1], it = 1.0f - t;
                px = it * it * x0 + 2 * t * it * x1 + t * t * x2;
                py = it * it * y0 + 2 * t * it * y1 + t * t * y2;
                dist2 = (px - sx) * (px - sx) + (py - sy) * (py - sy);
                if (dist2 < min_dist * min_dist) min_dist = (float)STBTT_sqrt(dist2);
              }
              if (num >= 3 && res[2] >= 0.0f && res[2] <= 1.0f) {
                t = res[2], it = 1.0f - t;
                px = it * it * x0 + 2 * t * it * x1 + t * t * x2;
                py = it * it * y0 + 2 * t * it * y1 + t * t * y2;
                dist2 = (px - sx) * (px - sx) + (py - sy) * (py - sy);
                if (dist2 < min_dist * min_dist) min_dist = (float)STBTT_sqrt(dist2);
              }
            }
          }
        }
        if (winding == 0) min_dist = -min_dist; // if outside the shape, value is negative
        val = onedge_value + pixel_dist_scale * min_dist;
        if (val < 0)
          val = 0;
        else if (val > 255)
          val = 255;
        data[(y - iy0) * w + (x - ix0)] = (unsigned char)val;
      }
    }
    STBTT_free(precompute, info->userdata);
    STBTT_free(verts, info->userdata);
  }
  return data;
}

STBTT_DEF unsigned char *stbtt_GetCodepointSDF(
  const stbtt_fontinfo *info,
  float scale,
  int codepoint,
  int padding,
  unsigned char onedge_value,
  float pixel_dist_scale,
  int *width,
  int *height,
  int *xoff,
  int *yoff) {
  return stbtt_GetGlyphSDF(
    info, scale, stbtt_FindGlyphIndex(info, codepoint), padding, onedge_value, pixel_dist_scale, width, height, xoff, yoff);
}

STBTT_DEF void stbtt_FreeSDF(unsigned char *bitmap, void *userdata) { STBTT_free(bitmap, userdata); }

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

STBTT_DEF int stbtt_InitFont(stbtt_fontinfo *info, const unsigned char *data, int offset) {
  return stbtt_InitFont_internal(info, (unsigned char *)data, offset);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif // STB_TRUETYPE_IMPLEMENTATION

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
