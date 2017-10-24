#pragma once
#include <string>
#include <vector>

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

    struct FONScontext;
}
#include "fontstash_impl.hpp"
