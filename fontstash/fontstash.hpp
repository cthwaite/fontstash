#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include <cmath>

#include <string>
#include <vector>

#ifndef FONS_SCRATCH_BUF_SIZE
#   define FONS_SCRATCH_BUF_SIZE 64000
#endif
#ifndef FONS_HASH_LUT_SIZE
#   define FONS_HASH_LUT_SIZE 256
#endif
#ifndef FONS_INIT_FONTS
#   define FONS_INIT_FONTS 4
#endif
#ifndef FONS_INIT_GLYPHS
#   define FONS_INIT_GLYPHS 256
#endif
#ifndef FONS_INIT_ATLAS_NODES
#   define FONS_INIT_ATLAS_NODES 256
#endif
#ifndef FONS_VERTEX_COUNT
#   define FONS_VERTEX_COUNT 1024
#endif
#ifndef FONS_MAX_STATES
#   define FONS_MAX_STATES 20
#endif
#ifndef FONS_MAX_FALLBACKS
#   define FONS_MAX_FALLBACKS 20
#endif

namespace fontstash {
    static constexpr int INVALID = -1;

    enum FONSflags {
        FONS_ZERO_TOPLEFT = 1,
        FONS_ZERO_BOTTOMLEFT = 2,
    };

    enum FONSalign {
        // Horizontal align
        FONS_ALIGN_LEFT     = 1<<0, // Default
        FONS_ALIGN_CENTER   = 1<<1,
        FONS_ALIGN_RIGHT    = 1<<2,
        // Vertical align
        FONS_ALIGN_TOP      = 1<<3,
        FONS_ALIGN_MIDDLE   = 1<<4,
        FONS_ALIGN_BOTTOM   = 1<<5,
        FONS_ALIGN_BASELINE = 1<<6, // Default
    };

    enum FONSerrorCode {
        // Font atlas is full.
        FONS_ATLAS_FULL = 1,
        // Scratch memory used to render glyphs is full, requested size reported in 'val', you may need to bump up FONS_SCRATCH_BUF_SIZE.
        FONS_SCRATCH_FULL = 2,
        // Calls to fonsPushState has created too large stack, if you need deep state stack bump up FONS_MAX_STATES.
        FONS_STATES_OVERFLOW = 3,
        // Trying to pop too many states fonsPopState().
        FONS_STATES_UNDERFLOW = 4,
    };

    struct FONSparams {
        FONSparams(int w, int h, unsigned char f) :
            width{w},
            height{h},
            flags{f}
        {
        }
        virtual ~FONSparams() = default;

        virtual int renderCreate(int width, int height) = 0;
        virtual int renderResize(int width, int height) = 0;
        virtual void renderUpdate(int* rect, const unsigned char* data) = 0;
        virtual void renderDraw( const float* verts, const float* tcoords, const unsigned int* colors, int nverts) = 0;
        virtual void renderDelete() = 0;

        int             width,
                        height;
        unsigned char   flags;
    };

    struct FONSquad {
        float x0,y0,s0,t0;
        float x1,y1,s1,t1;
    };

    struct FONScontext;
}
namespace fontstash {

    static FT_Library ftLibrary;

    struct FONSglyph {
        unsigned int codepoint;
        int index;
        int next;
        short size, blur;
        short x0,y0,x1,y1;
        short xadv,xoff,yoff;
    };


    struct FONSfont
    {
        FONSglyph* allocGlyph()
        {
            if (nglyphs + 1 > cglyphs)
            {
                cglyphs = cglyphs == 0 ? 8 : cglyphs * 2;
                glyphs = (FONSglyph*)realloc(glyphs, sizeof(FONSglyph) * cglyphs);
                if (glyphs == nullptr)
                {
                    return nullptr;
                }
            }
            nglyphs++;
            return &glyphs[nglyphs-1];
        }
        bool loadFont(FONScontext *context, const unsigned char *data, int dataSize)
        {
            (void)(context);

            //font->font.userdata = stash;
            FT_Error ft_error = FT_New_Memory_Face(ftLibrary, static_cast<const FT_Byte*>(data), dataSize, 0, &font_);
            return ft_error == 0;
        }
        void getFontVMetrics(int *ascent, int *descent, int *lineGap)
        {
            *ascent = font_->ascender;
            *descent = font_->descender;
            *lineGap = font_->height - (*ascent - *descent);
        }

        float getPixelHeightScale(float size)
        {
            return size / (font_->ascender - font_->descender);
        }

        int getGlyphIndex(int codepoint)
        {
            return FT_Get_Char_Index(font_, codepoint);
        }

        bool buildGlyphBitmap(int glyph, float size, float scale, int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1)
        {
            (void)(scale);
            FT_GlyphSlot ft_glyph;
            FT_Fixed adv_fixed;

            FT_Error ft_error = FT_Set_Pixel_Sizes(font_, 0, static_cast<FT_UInt>(size * static_cast<float>(font_->units_per_EM) / static_cast<float>(font_->ascender - font_->descender)));
            if (ft_error) return false;
            ft_error = FT_Load_Glyph(font_, glyph, FT_LOAD_RENDER);
            if (ft_error) return false;
            ft_error = FT_Get_Advance(font_, glyph, FT_LOAD_NO_SCALE, &adv_fixed);
            if (ft_error) return false;
            ft_glyph = font_->glyph;
            *advance = static_cast<int>(adv_fixed);
            *lsb = (int)ft_glyph->metrics.horiBearingX;
            *x0 = ft_glyph->bitmap_left;
            *x1 = *x0 + ft_glyph->bitmap.width;
            *y0 = -ft_glyph->bitmap_top;
            *y1 = *y0 + ft_glyph->bitmap.rows;
            return true;
        }

        void renderGlyphBitmap(unsigned char *output, int outWidth, int outHeight, int outStride, float scaleX, float scaleY, int glyph)
        {
            (void)(outWidth);
            (void)(outHeight);
            (void)(scaleX);
            (void)(scaleY);
            (void)(glyph);    // glyph has already been loaded by buildGlyphBitmap

            FT_GlyphSlot ft_glyph = font_->glyph;
            int ft_glyph_offset = 0;
            for(size_t y = 0; y < ft_glyph->bitmap.rows; y++ )
            {
                for(size_t x = 0; x < ft_glyph->bitmap.width; x++ )
                {
                    output[(y * outStride) + x] = ft_glyph->bitmap.buffer[ft_glyph_offset++];
                }
            }
        }

        int getGlyphKernAdvance(int glyph1, int glyph2)
        {
            FT_Vector ft_kerning;
            FT_Get_Kerning(font_, glyph1, glyph2, FT_KERNING_DEFAULT, &ft_kerning);
            return static_cast<int>((ft_kerning.x + 32) >> 6);  // Round up and convert to integer
        }

        char name[64];
        unsigned char* data;
        int dataSize;
        unsigned char freeData;
        float ascender;
        float descender;
        float lineh;
        FONSglyph* glyphs;
        int cglyphs;
        int nglyphs;
        int lut[FONS_HASH_LUT_SIZE];
        int fallbacks[FONS_MAX_FALLBACKS];
        int nfallbacks;
        FT_Face font_;
    };

    static bool initFreetype()
    {
        FT_Error ftError;
        ftError = FT_Init_FreeType(&ftLibrary);
        return ftError == 0;
    }

    struct FONStextIter {
        short       isize,
                    iblur;
        float       x, y, nextx, nexty, scale, spacing;
        int         prevGlyphIndex;
        unsigned int utf8state;
        unsigned int codepoint;
        FONSfont    *font;
        const char  *str;
        const char  *next;
        const char  *end;
    };
}

#include "fontstash_impl.hpp"
