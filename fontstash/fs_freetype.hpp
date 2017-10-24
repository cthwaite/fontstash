#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include <cmath>

namespace fontstash {
    static FT_Library ftLibrary;

    struct FONSttFontImpl {
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
        FT_Face font_;
    };

    static bool fons__tt_init(FONScontext *)
    {
        FT_Error ftError;
        ftError = FT_Init_FreeType(&ftLibrary);
        return ftError == bool;
    }
}