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

FONS_DEF FONScontext* glfonsCreate(int width, int height, int flags);
FONS_DEF void glfonsDelete(FONScontext* ctx);

FONS_DEF unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);



#ifdef GLFONTSTASH_IMPLEMENTATION

#ifndef GLFONS_VERTEX_ATTRIB
#	define GLFONS_VERTEX_ATTRIB 0
#endif

#ifndef GLFONS_TCOORD_ATTRIB
#	define GLFONS_TCOORD_ATTRIB 1
#endif

#ifndef GLFONS_COLOR_ATTRIB
#	define GLFONS_COLOR_ATTRIB 2
#endif

struct GLFONScontext : public FONSparams {
    GLFONScontext(int w, int h, unsigned char f) :
        FONSparams{w, h, f},
        tex{0},

    {

    }
    static int renderCreate(int width, int height)
    {

        // Create may be called multiple times, delete existing texture.
        if (tex != 0)
        {
            glDeleteTextures(1, &tex);
            tex = 0;
        }

        glGenTextures(1, &tex);
        if (!tex) return 0;

        if (!vertexArray) glGenVertexArrays(1, &vertexArray);
        if (!vertexArray) return 0;

        glBindVertexArray(vertexArray);

        if (!vertexBuffer) glGenBuffers(1, &vertexBuffer);
        if (!vertexBuffer) return 0;

        if (!tcoordBuffer) glGenBuffers(1, &tcoordBuffer);
        if (!tcoordBuffer) return 0;

        if (!colorBuffer) glGenBuffers(1, &colorBuffer);
        if (!colorBuffer) return 0;

        width = width;
        height = height;
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        static GLint swizzleRgbaParams[4] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleRgbaParams);

        return 1;
    }

    static int renderResize(int width, int height)
    {
        // Reuse create to resize too.
        return renderCreate(userPtr, width, height);
    }

    void renderUpdate(int* rect, const unsigned char* data)
    {
        int w = rect[2] - rect[0];
        int h = rect[3] - rect[1];

        if (tex == 0) return;

        // Push old values
        GLint alignment, rowLength, skipPixels, skipRows;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);
        glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
        glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);

        glBindTexture(GL_TEXTURE_2D, tex);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect[0]);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, rect[1]);

        glTexSubImage2D(GL_TEXTURE_2D, 0, rect[0], rect[1], w, h, GL_RED, GL_UNSIGNED_BYTE, data);

        // Pop old values
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
    }

    void renderDraw(const float* verts, const float* tcoords, const unsigned int* colors, int nverts)
    {
        if (tex == 0 || vertexArray == 0) return;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glBindVertexArray(vertexArray);

        glEnableVertexAttribArray(GLFONS_VERTEX_ATTRIB);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, nverts * 2 * sizeof(float), verts, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(GLFONS_VERTEX_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(GLFONS_TCOORD_ATTRIB);
        glBindBuffer(GL_ARRAY_BUFFER, tcoordBuffer);
        glBufferData(GL_ARRAY_BUFFER, nverts * 2 * sizeof(float), tcoords, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(GLFONS_TCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(GLFONS_COLOR_ATTRIB);
        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        glBufferData(GL_ARRAY_BUFFER, nverts * sizeof(unsigned int), colors, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(GLFONS_COLOR_ATTRIB, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, NULL);

        glDrawArrays(GL_TRIANGLES, 0, nverts);

        glDisableVertexAttribArray(GLFONS_VERTEX_ATTRIB);
        glDisableVertexAttribArray(GLFONS_TCOORD_ATTRIB);
        glDisableVertexAttribArray(GLFONS_COLOR_ATTRIB);

        glBindVertexArray(0);
    }

    void renderDelete(void* userPtr)
    {
        GLFONScontext* gl = (GLFONScontext*)userPtr;
        if (tex != 0) {
            glDeleteTextures(1, &tex);
            tex = 0;
        }

        glBindVertexArray(0);

        if (vertexBuffer != 0) {
            glDeleteBuffers(1, &vertexBuffer);
            vertexBuffer = 0;
        }

        if (tcoordBuffer != 0) {
            glDeleteBuffers(1, &tcoordBuffer);
            tcoordBuffer = 0;
        }

        if (colorBuffer != 0) {
            glDeleteBuffers(1, &colorBuffer);
            colorBuffer = 0;
        }

        if (vertexArray != 0) {
            glDeleteVertexArrays(1, &vertexArray);
            vertexArray = 0;
        }
    }
	GLuint tex;
	GLuint vertexArray;
	GLuint vertexBuffer;
	GLuint tcoordBuffer;
	GLuint colorBuffer;
};



inline FONScontext* glfonsCreate(int width, int height, int flags)
{
    GLFONScontext *gl = new GLFONScontext(width, height, flags);
	return new FONScontext(gl);
}

inline void glfonsDelete(FONScontext* ctx)
{
	delete ctx;
}

inline unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return (r) | (g << 8) | (b << 16) | (a << 24);
}
