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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#pragma once

namespace fontstash {
    struct GLFONScontext : FONSparams {
        GLFONScontext(int w, int h, unsigned char f) :
            FONSparams{w, h, f},
            tex{0}
        {
        }

        virtual ~GLFONScontext() = default;

        virtual int renderCreate(int w, int h)
        {
            // Create may be called multiple times, delete existing texture.
            if (tex != 0) {
                glDeleteTextures(1, &tex);
                tex = 0;
            }
            glGenTextures(1, &tex);
            if (!tex) return 0;
            width = w;
            height = h;
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            return 1;
        }

        virtual int renderResize(int width, int height)
        {
            // Reuse create to resize too.
            return renderCreate(width, height);
        }

        virtual void renderUpdate(int* rect, const unsigned char* data)
        {
            int w = rect[2] - rect[0];
            int h = rect[3] - rect[1];

            if (tex == 0) return;
            glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
            glBindTexture(GL_TEXTURE_2D, tex);
            glPixelStorei(GL_UNPACK_ALIGNMENT,1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect[0]);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, rect[1]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, rect[0], rect[1], w, h, GL_ALPHA,GL_UNSIGNED_BYTE, data);
            glPopClientAttrib();
        }

        virtual void renderDraw(const float* verts, const float* tcoords, const unsigned int* colors, int nverts)
        {
            if (tex == 0) return;
            glBindTexture(GL_TEXTURE_2D, tex);
            glEnable(GL_TEXTURE_2D);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            glVertexPointer(2, GL_FLOAT, sizeof(float)*2, verts);
            glTexCoordPointer(2, GL_FLOAT, sizeof(float)*2, tcoords);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(unsigned int), colors);

            glDrawArrays(GL_TRIANGLES, 0, nverts);

            glDisable(GL_TEXTURE_2D);
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
        }

        virtual void renderDelete()
        {
            if (tex != 0)
            {
                glDeleteTextures(1, &tex);
            }
            tex = 0;
        }
    	GLuint tex;
    };


    FONScontext* glfonsCreate(int width, int height, int flags)
    {
        GLFONScontext *params = new GLFONScontext(width, height, flags);
    	return new FONScontext(params);
    }

    void glfonsDelete(FONScontext* ctx)
    {
        delete ctx;
    }

    unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
    {
    	return (r) | (g << 8) | (b << 16) | (a << 24);
    }
}
