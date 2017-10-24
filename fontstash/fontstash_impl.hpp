//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//	claim that you wrote the original software. If you use this software
//	in a product, an acknowledgment in the product documentation would be
//	appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//	misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#pragma once
#include <cstdlib>
#include <iostream>
#include <vector>
#include <memory>
#include "fontstash/fs_util.hpp"
#include "fontstash/fs_utf8.hpp"
#include "fontstash/fs_atlas.hpp"
#include "fontstash/fs_blur.hpp"

namespace fontstash {
    using font_ptr = std::unique_ptr<FONSfont>;

    struct FONSstate {
        static constexpr size_t npos = ~size_t(0);
    	size_t font;
    	int align;
    	float size;
    	unsigned int color;
    	float blur;
    	float spacing;
        void clear()
        {
            size = 12.0f;
            color = 0xffffffff;
            font = FONSstate::npos;
            blur = 0;
            spacing = 0;
            align = FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE;
        }
    };


    struct FONScontext {
        FONScontext(FONSparams *p) :
            params{nullptr},
            handleError{nullptr},
            errorUptr{nullptr},
            nstates{0},
            nverts{0}
        {
            params.reset(p);
            // Allocate scratch buffer.
            scratch = (unsigned char*)std::calloc(FONS_SCRATCH_BUF_SIZE, sizeof(unsigned char));
            if (scratch == nullptr)
            {
                throw std::bad_alloc();
            }

            // Initialize implementation library
            if(!initFreetype())
            {
                throw std::runtime_error("Failed to initialise Freetype");
            }

            if (params->renderCreate(params->width, params->height) == 0)
            {
               throw std::runtime_error("Failed to initialise rendering backend");
            }

            atlas.reset(new FONSatlas(params->width, params->height, FONS_INIT_ATLAS_NODES));

            // Allocate space for fonts.
            fonts.resize(FONS_INIT_FONTS);

            // Create texture for the cache.
            itw_ = 1.0f/params->width;
            ith_ = 1.0f/params->height;
            texData = (unsigned char*)std::calloc(params->width * params->height, sizeof(unsigned char));
            if (texData == nullptr)
            {
                throw std::bad_alloc();
            }

            dirtyRect[0] = params->width;
            dirtyRect[1] = params->height;
            dirtyRect[2] = 0;
            dirtyRect[3] = 0;

            // Add white rect at 0,0 for debug drawing.
            addWhiteRect(2, 2);

            pushState();
            clearState();
        }

        ~FONScontext()
        {
            if(texData)
            {
                free(texData);
            }

            if(scratch)
            {
                free(scratch);
            }

            params->renderDelete();
        }

        void fonsSetErrorCallback(void (*callback)(void* uptr, int error, int val), void* uptr);
        // Returns current atlas size.
        void fonsGetAtlasSize(int* width, int* height);
        // Expands the atlas size.
        int fonsExpandAtlas(int width, int height);
        // Resets the whole stash.
        int fonsResetAtlas(int width, int height);

        // Add fonts
        int addFont(const char* name, const char* path);
        int addFontMem(const char* name, unsigned char* data, int ndata, int freeData);
        int getFontByName(const char* name);

        // State handling
        void pushState();
        void popState();
        void clearState();

        // State setting
        void setSize(float size);
        void setColor(unsigned int color);
        void setSpacing(float spacing);
        void setBlur(float blur);
        void setAlign(int align);
        void setFont(int font);

        // Draw text
        float fonsDrawText(float x, float y, const char* string, const char* end);

        // Measure text
        float textBounds(float x, float y, const char* string, const char* end, float* bounds);
        void fonsLineBounds(float y, float* miny, float* maxy);
        void vertMetrics(float* ascender, float* descender, float* lineh);

        // Text iterator
        int fonsTextIterInit(FONStextIter* iter, float x, float y, const char* str, const char* end);
        int fonsTextIterNext(FONStextIter* iter, struct FONSquad* quad);

        // Pull texture changes
        const unsigned char* fonsGetTextureData(int* width, int* height);
        int fonsValidateTexture(int* dirty);

        // Draws the stash texture for debugging
        void drawDebug(float x, float y);

        std::unique_ptr<FONSparams>  params;
    	float          itw_,
                        ith_;
    	unsigned char* texData;
    	int            dirtyRect[4];
        std::vector<font_ptr> fonts;
        std::unique_ptr<FONSatlas> atlas;
    	float           verts[FONS_VERTEX_COUNT*2];
    	float           tcoords[FONS_VERTEX_COUNT*2];
    	unsigned int    colors[FONS_VERTEX_COUNT];
    	int             nverts;
    	unsigned char   *scratch;
    	int             nscratch;
    	FONSstate       states[FONS_MAX_STATES];
    	int             nstates;
    	void            (*handleError)(void* uptr, int error, int val);
    	void            *errorUptr;

        void        getQuad(FONSfont *font, int prevGlyphIndex, FONSglyph* glyph, float scale, float spacing, float* x, float* y, FONSquad* q);

        void        addWhiteRect(int w, int h);
        int         addFallbackFont(int base, int fallback);
        void        flush();
        FONSstate*  getState()
        {
            return &states[nstates-1];
        }
        void freeFont(FONSfont *font)
        {
            if(font == nullptr) return;
            if(font->glyphs) free(font->glyphs);
            if(font->freeData && font->data) free(font->data);
            free(font);
        }

        int allocFont()
        {
            FONSfont *font = nullptr;
            font = reinterpret_cast<FONSfont*>(std::calloc(1, sizeof(FONSfont)));
            if(font == nullptr)
            {
                freeFont(font);
                return INVALID;
            }

            font->glyphs = reinterpret_cast<FONSglyph*>(std::calloc(FONS_INIT_GLYPHS, sizeof(FONSglyph)));
            if(font->glyphs == nullptr)
            {
                freeFont(font);
                return INVALID;
            }
            font->cglyphs = FONS_INIT_GLYPHS;
            font->nglyphs = 0;

            fonts.emplace_back(font);
            return fonts.size() - 1;
        }

        void vertex(float x, float y, float s, float t, unsigned int c)
        {
            verts[nverts*2+0] = x;
            verts[nverts*2+1] = y;
            tcoords[nverts*2+0] = s;
            tcoords[nverts*2+1] = t;
            colors[nverts] = c;
            nverts++;
        }
    };


    #ifdef STB_TRUETYPE_IMPLEMENTATION

    static void* fons__tmpalloc(size_t size, void* up)
    {
    	unsigned char* ptr;
    	FONScontext* stash = (FONScontext*)up;

    	// 16-byte align the returned pointer
    	size = (size + 0xf) & ~0xf;

    	if(stash->nscratch+(int)size > FONS_SCRATCH_BUF_SIZE)
        {
    		if(stash->handleError)
            {
    			stash->handleError(stash->errorUptr, FONS_SCRATCH_FULL, stash->nscratch+(int)size);
    		}
            return nullptr;
    	}
    	ptr = stash->scratch + stash->nscratch;
    	stash->nscratch += (int)size;
    	return ptr;
    }

    static void fons__tmpfree(void* ptr, void* up)
    {
    	(void)ptr;
    	(void)up;
    	// empty
    }

    #endif // STB_TRUETYPE_IMPLEMENTATION


    void FONScontext::addWhiteRect(int w, int h)
    {
        int x, y, gx, gy;
        if(atlas->addRect(w, h, &gx, &gy) == 0)
        {
            return;
        }
        // Rasterize
        unsigned char *dst = &texData[gx + gy * params->width];
        for (y = 0; y < h; y++)
        {
            for (x = 0; x < w; x++)
            {
                dst[x] = 0xff;
            }
            dst += params->width;
        }

        dirtyRect[0] = mini(dirtyRect[0], gx);
        dirtyRect[1] = mini(dirtyRect[1], gy);
        dirtyRect[2] = maxi(dirtyRect[2], gx+w);
        dirtyRect[3] = maxi(dirtyRect[3], gy+h);
    }


    int FONScontext::addFallbackFont(int base, int fallback)
    {
    	FONSfont *baseFont = fonts[base].get();
    	if(baseFont->nfallbacks < FONS_MAX_FALLBACKS) {
    		baseFont->fallbacks[baseFont->nfallbacks++] = fallback;
    		return 1;
    	}
    	return 0;
    }

    void FONScontext::setSize(float size)
    {
    	getState()->size = size;
    }

    void FONScontext::setColor(unsigned int color)
    {
    	getState()->color = color;
    }

    void FONScontext::setSpacing(float spacing)
    {
    	getState()->spacing = spacing;
    }

    void FONScontext::setBlur(float blur)
    {
    	getState()->blur = blur;
    }

    void FONScontext::setAlign(int align)
    {
    	getState()->align = align;
    }

    void FONScontext::setFont(int font)
    {
    	getState()->font = font;
    }

    void FONScontext::pushState()
    {
    	if(nstates >= FONS_MAX_STATES)
        {
    		if(handleError)
            {
    			handleError(errorUptr, FONS_STATES_OVERFLOW, 0);
    		}
            return;
    	}
    	if(nstates > 0)
        {
            std::cout << "memcpy state" << std::endl;
    		memcpy(&states[nstates], &states[nstates-1], sizeof(FONSstate));
    	}
        ++nstates;
    }

    void FONScontext::popState()
    {
    	if(nstates <= 1)
        {
    		if(handleError)
            {
    			handleError(errorUptr, FONS_STATES_UNDERFLOW, 0);
    		}
            return;
    	}
    	nstates--;
    }

    void FONScontext::clearState()
    {
        getState()->clear();
    }

    static FILE* fons__fopen(const char* filename, const char* mode)
    {
    #ifdef _WIN32
    	int len = 0;
    	int fileLen = strlen(filename);
    	int modeLen = strlen(mode);
    	wchar_t wpath[MAX_PATH];
    	wchar_t wmode[MAX_PATH];
    	FILE* f;

    	if(fileLen == 0)
    		return nullptr;
    	if(modeLen == 0)
    		return nullptr;
    	len = MultiByteToWideChar(CP_UTF8, 0, filename, fileLen, wpath, fileLen);
    	if(len >= MAX_PATH)
    		return nullptr;
    	wpath[len] = L'\0';
    	len = MultiByteToWideChar(CP_UTF8, 0, mode, modeLen, wmode, modeLen);
    	if(len >= MAX_PATH)
    		return nullptr;
    	wmode[len] = L'\0';
    	f = _wfopen(wpath, wmode);
    	return f;
    #else
    	return fopen(filename, mode);
    #endif
    }

    int FONScontext::addFont(const char* name, const char* path)
    {
    	FILE* fp = 0;
    	int dataSize = 0, readed;
    	unsigned char* data = nullptr;

    	// Read in the font data.
    	fp = fons__fopen(path, "rb");
    	if(fp == nullptr) goto error;
    	fseek(fp,0,SEEK_END);
    	dataSize = (int)ftell(fp);
    	fseek(fp,0,SEEK_SET);
    	data = (unsigned char*)std::calloc(dataSize, sizeof(unsigned char));
    	if(data == nullptr) goto error;
    	readed = fread(data, 1, dataSize, fp);
    	fclose(fp);
    	fp = 0;
    	if(readed != dataSize) goto error;

    	return addFontMem(name, data, dataSize, 1);

    error:
    	if(data) free(data);
    	if(fp) fclose(fp);
    	return INVALID;
    }

    int FONScontext::addFontMem(const char* name, unsigned char* data, int dataSize, int freeData)
    {
    	int i, ascent, descent, fh, lineGap;
    	
    	int idx = allocFont();
    	if(idx == INVALID)
        {
    		return INVALID;
        }

    	FONSfont *font = fonts[idx].get();

    	strncpy(font->name, name, sizeof(font->name));
    	font->name[sizeof(font->name)-1] = '\0';

    	// Init hash lookup.
    	for (i = 0; i < FONS_HASH_LUT_SIZE; ++i)
        {
    		font->lut[i] = -1;
        }

    	// Read in the font data.
    	font->dataSize = dataSize;
    	font->data = data;
    	font->freeData = static_cast<unsigned char>(freeData);

    	// Init font
    	nscratch = 0;
    	if(!font->loadFont(this, data, dataSize))
        {
            freeFont(font);
            fonts.pop_back();
            return INVALID;
        }
    	// Store normalized line height. The real line height is got
    	// by multiplying the lineh by font size.
    	font->getFontVMetrics(&ascent, &descent, &lineGap);
    	fh = ascent - descent;
    	font->ascender = (float)ascent / (float)fh;
    	font->descender = (float)descent / (float)fh;
    	font->lineh = (float)(fh + lineGap) / (float)fh;

    	return idx;
    }

    int FONScontext::getFontByName(const char* name)
    {
    	for(size_t index = 0; index < fonts.size(); ++index)
        {
    		if(strcmp(fonts[index]->name, name) == 0)
            {
    			return index;
            }
    	}
    	return INVALID;
    }



    static FONSglyph* fons__getGlyph(FONScontext* stash, FONSfont *font, unsigned int codepoint,
    								 short isize, short iblur)
    {
    	int i, g, advance, lsb, x0, y0, x1, y1, gw, gh, gx, gy, x, y;
    	float scale;
    	FONSglyph* glyph = nullptr;
    	unsigned int h;
    	float size = isize/10.0f;
    	int pad, added;
    	unsigned char* bdst;
    	unsigned char* dst;
    	FONSfont *renderFont = font;

    	if(isize < 2) return nullptr;
    	if(iblur > 20) iblur = 20;
    	pad = iblur+2;

    	// Reset allocator.
    	stash->nscratch = 0;

    	// Find code point and size.
    	h = hashint(codepoint) & (FONS_HASH_LUT_SIZE-1);
    	i = font->lut[h];
    	while (i != -1) {
    		if(font->glyphs[i].codepoint == codepoint && font->glyphs[i].size == isize && font->glyphs[i].blur == iblur)
            {
    			return &font->glyphs[i];
    		}
            i = font->glyphs[i].next;
    	}

    	// Could not find glyph, create it.
    	g = font->getGlyphIndex(codepoint);
    	// Try to find the glyph in fallback fonts.
    	if(g == 0)
        {
    		for (i = 0; i < font->nfallbacks; ++i)
            {
    			FONSfont *fallbackFont = stash->fonts[font->fallbacks[i]].get();
    			int fallbackIndex = fallbackFont->getGlyphIndex(codepoint);
    			if(fallbackIndex != 0)
                {
    				g = fallbackIndex;
    				renderFont = fallbackFont;
    				break;
    			}
    		}
    		// It is possible that we did not find a fallback glyph.
    		// In that case the glyph index 'g' is 0, and we'll proceed below and cache empty glyph.
    	}
    	scale = renderFont->getPixelHeightScale(size);
    	renderFont->buildGlyphBitmap(g, size, scale, &advance, &lsb, &x0, &y0, &x1, &y1);
    	gw = x1-x0 + pad*2;
    	gh = y1-y0 + pad*2;

    	// Find free spot for the rect in the atlas
    	added = stash->atlas->addRect(gw, gh, &gx, &gy);
    	if(added == 0 && stash->handleError != nullptr) {
    		// Atlas is full, let the user to resize the atlas (or not), and try again.
    		stash->handleError(stash->errorUptr, FONS_ATLAS_FULL, 0);
    		added = stash->atlas->addRect(gw, gh, &gx, &gy);
    	}
    	if(added == 0) return nullptr;

    	// Init glyph.
    	glyph = font->allocGlyph();
    	glyph->codepoint = codepoint;
    	glyph->size = isize;
    	glyph->blur = iblur;
    	glyph->index = g;
    	glyph->x0 = (short)gx;
    	glyph->y0 = (short)gy;
    	glyph->x1 = (short)(glyph->x0+gw);
    	glyph->y1 = (short)(glyph->y0+gh);
    	glyph->xadv = (short)(scale * advance * 10.0f);
    	glyph->xoff = (short)(x0 - pad);
    	glyph->yoff = (short)(y0 - pad);
    	glyph->next = 0;

    	// Insert char to hash lookup.
    	glyph->next = font->lut[h];
    	font->lut[h] = font->nglyphs-1;

    	// Rasterize
    	dst = &stash->texData[(glyph->x0+pad) + (glyph->y0+pad) * stash->params->width];
    	renderFont->renderGlyphBitmap(dst, gw-pad*2,gh-pad*2, stash->params->width, scale,scale, g);

    	// Make sure there is one pixel empty border.
    	dst = &stash->texData[glyph->x0 + glyph->y0 * stash->params->width];
    	for (y = 0; y < gh; y++) {
    		dst[y*stash->params->width] = 0;
    		dst[gw-1 + y*stash->params->width] = 0;
    	}
    	for (x = 0; x < gw; x++) {
    		dst[x] = 0;
    		dst[x + (gh-1)*stash->params->width] = 0;
    	}

    	// Debug code to color the glyph background
    /*	unsigned char* fdst = &stash->texData[glyph->x0 + glyph->y0 * stash->params->width];
    	for (y = 0; y < gh; y++) {
    		for (x = 0; x < gw; x++) {
    			int a = (int)fdst[x+y*stash->params->width] + 20;
    			if(a > 255) a = 255;
    			fdst[x+y*stash->params->width] = a;
    		}
    	}*/

    	// Blur
    	if(iblur > 0)
        {
    		stash->nscratch = 0;
    		bdst = &stash->texData[glyph->x0 + glyph->y0 * stash->params->width];
            fontstash::blur(bdst, gw,gh, stash->params->width, iblur);
    	}

    	stash->dirtyRect[0] = mini(stash->dirtyRect[0], glyph->x0);
    	stash->dirtyRect[1] = mini(stash->dirtyRect[1], glyph->y0);
    	stash->dirtyRect[2] = maxi(stash->dirtyRect[2], glyph->x1);
    	stash->dirtyRect[3] = maxi(stash->dirtyRect[3], glyph->y1);

    	return glyph;
    }

    void FONScontext::getQuad(FONSfont *font, int prevGlyphIndex, FONSglyph* glyph, float scale, float spacing, float* x, float* y, FONSquad* q)
    {
    	float rx,ry,xoff,yoff,x0,y0,x1,y1;

    	if(prevGlyphIndex != -1) {
    		float adv = font->getGlyphKernAdvance(prevGlyphIndex, glyph->index) * scale;
    		*x += (int)(adv + spacing + 0.5f);
    	}

    	// Each glyph has 2px border to allow good interpolation,
    	// one pixel to prevent leaking, and one to allow good interpolation for rendering.
    	// Inset the texture region by one pixel for correct interpolation.
    	xoff = (short)(glyph->xoff+1);
    	yoff = (short)(glyph->yoff+1);
    	x0 = (float)(glyph->x0+1);
    	y0 = (float)(glyph->y0+1);
    	x1 = (float)(glyph->x1-1);
    	y1 = (float)(glyph->y1-1);

    	if(params->flags & FONS_ZERO_TOPLEFT) {
    		rx = (float)(int)(*x + xoff);
    		ry = (float)(int)(*y + yoff);

    		q->x0 = rx;
    		q->y0 = ry;
    		q->x1 = rx + x1 - x0;
    		q->y1 = ry + y1 - y0;

    		q->s0 = x0 * itw_;
    		q->t0 = y0 * ith_;
    		q->s1 = x1 * itw_;
    		q->t1 = y1 * ith_;
    	} else {
    		rx = (float)(int)(*x + xoff);
    		ry = (float)(int)(*y - yoff);

    		q->x0 = rx;
    		q->y0 = ry;
    		q->x1 = rx + x1 - x0;
    		q->y1 = ry - y1 + y0;

    		q->s0 = x0 * itw_;
    		q->t0 = y0 * ith_;
    		q->s1 = x1 * itw_;
    		q->t1 = y1 * ith_;
    	}

    	*x += (int)(glyph->xadv / 10.0f + 0.5f);
    }

    void FONScontext::flush()
    {
    	// Flush texture
    	if(dirtyRect[0] < dirtyRect[2] && dirtyRect[1] < dirtyRect[3]) {
            params->renderUpdate(dirtyRect, texData);
    		// Reset dirty rect
    		dirtyRect[0] = params->width;
    		dirtyRect[1] = params->height;
    		dirtyRect[2] = 0;
    		dirtyRect[3] = 0;
    	}

    	// Flush triangles
    	if(nverts > 0)
        {
            params->renderDraw(verts, tcoords, colors, nverts);
    		nverts = 0;
    	}
    }
    static float fons__getVertAlign(FONScontext* stash, FONSfont *font, int align, short isize)
    {
    	if(stash->params->flags & FONS_ZERO_TOPLEFT) {
    		if(align & FONS_ALIGN_TOP) {
    			return font->ascender * (float)isize/10.0f;
    		} else if(align & FONS_ALIGN_MIDDLE) {
    			return (font->ascender + font->descender) / 2.0f * (float)isize/10.0f;
    		} else if(align & FONS_ALIGN_BASELINE) {
    			return 0.0f;
    		} else if(align & FONS_ALIGN_BOTTOM) {
    			return font->descender * (float)isize/10.0f;
    		}
    	} else {
    		if(align & FONS_ALIGN_TOP) {
    			return -font->ascender * (float)isize/10.0f;
    		} else if(align & FONS_ALIGN_MIDDLE) {
    			return -(font->ascender + font->descender) / 2.0f * (float)isize/10.0f;
    		} else if(align & FONS_ALIGN_BASELINE) {
    			return 0.0f;
    		} else if(align & FONS_ALIGN_BOTTOM) {
    			return -font->descender * (float)isize/10.0f;
    		}
    	}
    	return 0.0;
    }

    float FONScontext::fonsDrawText(float x, float y, const char* str, const char* end)
    {
    	FONSstate* state = getState();
    	unsigned int codepoint;
    	unsigned int utf8state = 0;
    	FONSglyph* glyph = nullptr;
    	FONSquad q;
    	int prevGlyphIndex = -1;
    	short isize = (short)(state->size*10.0f);
    	short iblur = static_cast<short>(state->blur);
    	float scale;
    	float width;

    	if(state->font == FONSstate::npos || state->font >= fonts.size() - 1) return x;
    	FONSfont *font = fonts[state->font].get();
    	if(font->data == nullptr) return x;

    	scale = font->getPixelHeightScale(static_cast<float>(isize)/10.0f);

    	if(end == nullptr)
    		end = str + strlen(str);

    	// Align horizontally
    	if(state->align & FONS_ALIGN_LEFT) {
    		// empty
    	} else if(state->align & FONS_ALIGN_RIGHT) {
    		width = textBounds(x,y, str, end, nullptr);
    		x -= width;
    	} else if(state->align & FONS_ALIGN_CENTER) {
    		width = textBounds(x,y, str, end, nullptr);
    		x -= width * 0.5f;
    	}
    	// Align vertically.
    	y += fons__getVertAlign(this, font, state->align, isize);

    	for (; str != end; ++str)
        {
    		if(fontstash::decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
    			continue;
    		glyph = fons__getGlyph(this, font, codepoint, isize, iblur);
    		if(glyph != nullptr) {
    			getQuad(font, prevGlyphIndex, glyph, scale, state->spacing, &x, &y, &q);

    			if(nverts+6 > FONS_VERTEX_COUNT)
    				flush();

    			vertex(q.x0, q.y0, q.s0, q.t0, state->color);
    			vertex(q.x1, q.y1, q.s1, q.t1, state->color);
    			vertex(q.x1, q.y0, q.s1, q.t0, state->color);

    			vertex(q.x0, q.y0, q.s0, q.t0, state->color);
    			vertex(q.x0, q.y1, q.s0, q.t1, state->color);
    			vertex(q.x1, q.y1, q.s1, q.t1, state->color);
    		}
    		prevGlyphIndex = glyph != nullptr ? glyph->index : -1;
    	}
    	flush();

    	return x;
    }

    int FONScontext::fonsTextIterInit(FONStextIter* iter, float x, float y, const char* str, const char* end)
    {
    	FONSstate* state = getState();
    	float width;

    	memset(iter, 0, sizeof(*iter));

    	if(state->font == FONSstate::npos || state->font >= fonts.size() - 1)
        {
            return 0;
        }
    	iter->font = fonts[state->font].get();
    	if(iter->font->data == nullptr) return 0;

    	iter->isize = (short)(state->size*10.0f);
    	iter->iblur = (short)state->blur;
    	iter->scale = iter->font->getPixelHeightScale((float)iter->isize/10.0f);

    	// Align horizontally
    	if(state->align & FONS_ALIGN_LEFT) {
    		// empty
    	} else if(state->align & FONS_ALIGN_RIGHT) {
    		width = textBounds(x,y, str, end, nullptr);
    		x -= width;
    	} else if(state->align & FONS_ALIGN_CENTER) {
    		width = textBounds(x,y, str, end, nullptr);
    		x -= width * 0.5f;
    	}
    	// Align vertically.
    	y += fons__getVertAlign(this, iter->font, state->align, iter->isize);

    	if(end == nullptr)
    		end = str + strlen(str);

    	iter->x = iter->nextx = x;
    	iter->y = iter->nexty = y;
    	iter->spacing = state->spacing;
    	iter->str = str;
    	iter->next = str;
    	iter->end = end;
    	iter->codepoint = 0;
    	iter->prevGlyphIndex = -1;

    	return 1;
    }

    int FONScontext::fonsTextIterNext(FONStextIter* iter, FONSquad* quad)
    {
    	FONSglyph* glyph = nullptr;
    	const char* str = iter->next;
    	iter->str = iter->next;

    	if(str == iter->end)
        {
    		return 0;
        }

    	for (; str != iter->end; str++)
        {
    		if(fontstash::decutf8(&iter->utf8state, &iter->codepoint, *(const unsigned char*)str))
    			continue;
    		str++;
    		// Get glyph and quad
    		iter->x = iter->nextx;
    		iter->y = iter->nexty;
    		glyph = fons__getGlyph(this, iter->font, iter->codepoint, iter->isize, iter->iblur);
    		if(glyph != nullptr)
            {
    			getQuad(iter->font, iter->prevGlyphIndex, glyph, iter->scale, iter->spacing, &iter->nextx, &iter->nexty, quad);
            }
    		iter->prevGlyphIndex = glyph != nullptr ? glyph->index : -1;

    		break;
    	}
    	iter->next = str;

    	return 1;
    }

    void FONScontext::drawDebug(float x, float y)
    {
    	int w = params->width;
    	int h = params->height;
    	float u = w == 0 ? 0 : (1.0f / w);
    	float v = h == 0 ? 0 : (1.0f / h);

    	if(nverts+6+6 > FONS_VERTEX_COUNT)
    		flush();

    	// Draw background
    	vertex(x+0, y+0, u, v, 0x0fffffff);
    	vertex(x+w, y+h, u, v, 0x0fffffff);
    	vertex(x+w, y+0, u, v, 0x0fffffff);

    	vertex(x+0, y+0, u, v, 0x0fffffff);
    	vertex(x+0, y+h, u, v, 0x0fffffff);
    	vertex(x+w, y+h, u, v, 0x0fffffff);

    	// Draw texture
    	vertex(x+0, y+0, 0, 0, 0xffffffff);
    	vertex(x+w, y+h, 1, 1, 0xffffffff);
    	vertex(x+w, y+0, 1, 0, 0xffffffff);

    	vertex(x+0, y+0, 0, 0, 0xffffffff);
    	vertex(x+0, y+h, 0, 1, 0xffffffff);
    	vertex(x+w, y+h, 1, 1, 0xffffffff);

    	// Drawbug draw atlas
    	for(int i = 0; i < atlas->nnodes(); i++)
        {
    		FONSatlasNode* n = &atlas->nodes_[i];

    		if(nverts + 6 > FONS_VERTEX_COUNT)
            {
    			flush();
            }

    		vertex(x+n->x+0, y+n->y+0, u, v, 0xc00000ff);
    		vertex(x+n->x+n->width, y+n->y+1, u, v, 0xc00000ff);
    		vertex(x+n->x+n->width, y+n->y+0, u, v, 0xc00000ff);

    		vertex(x+n->x+0, y+n->y+0, u, v, 0xc00000ff);
    		vertex(x+n->x+0, y+n->y+1, u, v, 0xc00000ff);
    		vertex(x+n->x+n->width, y+n->y+1, u, v, 0xc00000ff);
    	}

    	flush();
    }

    float FONScontext::textBounds(float x, float y, const char* str, const char* end, float* bounds)
    {
    	FONSstate* state = getState();
    	unsigned int codepoint;
    	unsigned int utf8state = 0;
    	FONSquad q;
    	FONSglyph* glyph = nullptr;
    	int prevGlyphIndex = -1;
    	short isize = (short)(state->size*10.0f);
    	short iblur = (short)state->blur;
    	float scale;
    	float startx, advance;

    	if(state->font == FONSstate::npos || state->font >= fonts.size() - 1) return 0;
    	FONSfont *font = fonts[state->font].get();
    	if(font->data == nullptr) return 0;

    	scale = font->getPixelHeightScale(static_cast<float>(isize)/10.0f);

    	// Align vertically.
    	y += fons__getVertAlign(this, font, state->align, isize);

    	float minx, miny, maxx, maxy;
    	minx = maxx = x;
    	miny = maxy = y;
    	startx = x;

    	if(end == nullptr)
        {
    		end = str + strlen(str);
        }

    	for (; str != end; ++str)
        {
    		if(fontstash::decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
            {
    			continue;
            }
    		glyph = fons__getGlyph(this, font, codepoint, isize, iblur);
    		if(glyph != nullptr) {
    			getQuad(font, prevGlyphIndex, glyph, scale, state->spacing, &x, &y, &q);
    			if(q.x0 < minx) minx = q.x0;
    			if(q.x1 > maxx) maxx = q.x1;
    			if(params->flags & FONS_ZERO_TOPLEFT)
                {
    				if(q.y0 < miny) miny = q.y0;
    				if(q.y1 > maxy) maxy = q.y1;
    			}
                else
                {
    				if(q.y1 < miny) miny = q.y1;
    				if(q.y0 > maxy) maxy = q.y0;
    			}
    		}
    		prevGlyphIndex = glyph != nullptr ? glyph->index : -1;
    	}

    	advance = x - startx;

    	// Align horizontally
    	if(state->align & FONS_ALIGN_LEFT) {
    		// empty
    	} else if(state->align & FONS_ALIGN_RIGHT) {
    		minx -= advance;
    		maxx -= advance;
    	} else if(state->align & FONS_ALIGN_CENTER) {
    		minx -= advance * 0.5f;
    		maxx -= advance * 0.5f;
    	}

    	if(bounds)
        {
    		bounds[0] = minx;
    		bounds[1] = miny;
    		bounds[2] = maxx;
    		bounds[3] = maxy;
    	}

    	return advance;
    }

    void FONScontext::vertMetrics(float* ascender, float* descender, float* lineh)
    {
    	FONSstate* state = getState();
    	short isize;

    	if(state->font == FONSstate::npos || state->font >= fonts.size() - 1)
        {
            return;
        }
    	FONSfont *font = fonts[state->font].get();
    	isize = (short)(state->size*10.0f);
    	if(font->data == nullptr)
        {
            return;
        }

    	if(ascender)
        {
    		*ascender = font->ascender*isize/10.0f;
        }
    	if(descender)
        {
    		*descender = font->descender*isize/10.0f;
        }
    	if(lineh)
        {
    		*lineh = font->lineh*isize/10.0f;
        }
    }

    void FONScontext::fonsLineBounds(float y, float* miny, float* maxy)
    {
    	FONSstate* state = getState();
    	short isize;

    	if(state->font == FONSstate::npos || state->font >= fonts.size() - 1)
        {
            return;
        }
    	FONSfont *font = fonts[state->font].get();
    	isize = static_cast<short>(state->size*10.0f);
    	if(font->data == nullptr)
        {
            return;
        }

    	y += fons__getVertAlign(this, font, state->align, isize);

    	if(params->flags & FONS_ZERO_TOPLEFT)
        {
    		*miny = y - font->ascender * static_cast<float>(isize) / 10.0f;
    		*maxy = *miny + font->lineh * isize / 10.0f;
    	}
        else
        {
    		*maxy = y + font->descender * static_cast<float>(isize) / 10.0f;
    		*miny = *maxy - font->lineh * isize / 10.0f;
    	}
    }

    const unsigned char* FONScontext::fonsGetTextureData(int* width, int* height)
    {
    	if(width != nullptr)
        {
    		*width = params->width;
        }
    	if(height != nullptr)
    	{
        	*height = params->height;
        }
    	return texData;
    }

    int FONScontext::fonsValidateTexture(int* dirty)
    {
    	if(dirtyRect[0] < dirtyRect[2] && dirtyRect[1] < dirtyRect[3])
        {
    		dirty[0] = dirtyRect[0];
    		dirty[1] = dirtyRect[1];
    		dirty[2] = dirtyRect[2];
    		dirty[3] = dirtyRect[3];
    		// Reset dirty rect
    		dirtyRect[0] = params->width;
    		dirtyRect[1] = params->height;
    		dirtyRect[2] = 0;
    		dirtyRect[3] = 0;
    		return 1;
    	}
    	return 0;
    }

    inline void FONScontext::fonsSetErrorCallback(void (*callback)(void* uptr, int error, int val), void* uptr)
    {
    	handleError = callback;
    	errorUptr = uptr;
    }

    inline void FONScontext::fonsGetAtlasSize(int* width, int* height)
    {
    	*width = params->width;
    	*height = params->height;
    }

    int FONScontext::fonsExpandAtlas(int width, int height)
    {
    	int maxy = 0;

    	width = maxi(width, params->width);
    	height = maxi(height, params->height);

    	if(width == params->width && height == params->height)
        {
    		return 1;
        }

    	// Flush pending glyphs.
    	flush();

    	// Create new texture
        if(params->renderResize(width, height) == 0)
        {
    			return 0;
    	}
    	// Copy old texture data over.
    	unsigned char *data = reinterpret_cast<unsigned char*>(std::calloc(width * height, sizeof(unsigned char)));
    	if(data == nullptr)
        {
    		return 0;
        }
    	for(int i = 0; i < params->height; ++i)
        {
    		unsigned char *dst = &data[i*width];
    		unsigned char *src = &texData[i*params->width];
    		memcpy(dst, src, params->width);
    		if(width > params->width)
            {
    			memset(dst+params->width, 0, width - params->width);
            }
    	}
    	if(height > params->height)
        {
    		memset(&data[params->height * width], 0, (height - params->height) * width);
        }

    	free(texData);
    	texData = data;

    	// Increase atlas size
    	atlas->expand(width, height);

    	// Add existing data as dirty.
    	for (int i = 0; i < atlas->nnodes(); ++i)
        {
    		maxy = maxi(maxy, atlas->nodes_[i].y);
        }
    	dirtyRect[0] = 0;
    	dirtyRect[1] = 0;
    	dirtyRect[2] = params->width;
    	dirtyRect[3] = maxy;

    	params->width = width;
    	params->height = height;
    	itw_ = 1.0f / params->width;
    	ith_ = 1.0f / params->height;

    	return 1;
    }

    inline int FONScontext::fonsResetAtlas(int width, int height)
    {
    	// Flush pending glyphs.
    	flush();

    	// Create new texture
        if(params->renderResize(width, height) == 0)
        {
    			return 0;
    	}

    	// Reset atlas
    	atlas->reset(width, height);

    	// Clear texture data.
    	texData = (unsigned char*)realloc(texData, width * height);
    	if(texData == nullptr)
        {
            return 0;
        }
    	memset(texData, 0, width * height);

    	// Reset dirty rect
    	dirtyRect[0] = width;
    	dirtyRect[1] = height;
    	dirtyRect[2] = 0;
    	dirtyRect[3] = 0;

    	// Reset cached glyphs
    	for(const auto &font : fonts)
        {
    		font->nglyphs = 0;
    		for (int j = 0; j < FONS_HASH_LUT_SIZE; j++)
    	   	{
                font->lut[j] = -1;
            }
    	}

    	params->width = width;
    	params->height = height;
    	itw_ = 1.0f/params->width;
    	ith_ = 1.0f/params->height;

    	// Add white rect at 0,0 for debug drawing.
    	addWhiteRect(2, 2);

    	return 1;
    }
}
