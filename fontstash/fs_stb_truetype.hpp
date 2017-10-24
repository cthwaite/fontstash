
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC

namespace fontstash {
static void* fons__tmpalloc(size_t size, void* up);
static void fons__tmpfree(void* ptr, void* up);
}
#define STBTT_malloc(x,u)    fontstash::fons__tmpalloc(x,u)
#define STBTT_free(x,u)      fontstash::fons__tmpfree(x,u)
#include "stb_truetype.h"

namespace fontstash {
    struct FONSttFontImpl {
        int loadFont(FONScontext *context, unsigned char *data, int dataSize)
        {
            int stbError;
            (void)(dataSize);

            font_.userdata = context;
            stbError = stbtt_InitFont(&font_, data, 0);
            return stbError;
        }

        void getFontVMetrics(int *ascent, int *descent, int *lineGap)
        {
            stbtt_GetFontVMetrics(&font_, ascent, descent, lineGap);
        }

        float getPixelHeightScale(float size)
        {
            return stbtt_ScaleForPixelHeight(&font_, size);
        }

        int getGlyphIndex(int codepoint)
        {
            return stbtt_FindGlyphIndex(&font_, codepoint);
        }

        int buildGlyphBitmap(int glyph, float size, float scale,
                                      int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1)
        {
            (void)(size);
            stbtt_GetGlyphHMetrics(&font_, glyph, advance, lsb);
            stbtt_GetGlyphBitmapBox(&font_, glyph, scale, scale, x0, y0, x1, y1);
            return 1;
        }

        void renderGlyphBitmap(unsigned char *output, int outWidth, int outHeight, int outStride,
                                        float scaleX, float scaleY, int glyph)
        {
            stbtt_MakeGlyphBitmap(&font_, output, outWidth, outHeight, outStride, scaleX, scaleY, glyph);
        }

        int getGlyphKernAdvance(int glyph1, int glyph2)
        {
            return stbtt_GetGlyphKernAdvance(&font_, glyph1, glyph2);
        }
        stbtt_fontinfo font_;
    };

    static bool fons__tt_init(FONScontext *context)
    {
        (void)(context);
        return true;
    }
}
