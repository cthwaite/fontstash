//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
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

#include <stdio.h>
#include <string.h>
#define FONTSTASH_IMPLEMENTATION

// #define FONS_USE_FREETYPE

#include "fontstash.hpp"
#include "GLFW/glfw3.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.hpp"

namespace fs = fontstash;

int debug = 0;

void dash(float dx, float dy)
{
	glBegin(GL_LINES);
	glColor4ub(0,0,0,128);
	glVertex2f(dx-5,dy);
	glVertex2f(dx-10,dy);
	glEnd();
}

void line(float sx, float sy, float ex, float ey)
{
	glBegin(GL_LINES);
	glColor4ub(0,0,0,128);
	glVertex2f(sx,sy);
	glVertex2f(ex,ey);
	glEnd();
}

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode;
	(void)mods;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		debug = !debug;
}

int main()
{
	int fontNormal = fs::INVALID;
	int fontItalic = fs::INVALID;
	int fontBold = fs::INVALID;
	int fontJapanese = fs::INVALID;

	if (!glfwInit())
    {
        return -1;
    }


	const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    GLFWwindow *window = glfwCreateWindow(mode->width - 40, mode->height - 80, "Font Stash", NULL, NULL);
	if (!window)
    {
		glfwTerminate();
		return -1;
	}

    glfwSetKeyCallback(window, key);
	glfwMakeContextCurrent(window);

	fs::FONScontext *sh = fs::glfonsCreate(512, 512, fs::FONS_ZERO_TOPLEFT);
	if (sh == NULL) {
		printf("Could not create stash.\n");
		return -1;
	}

	fontNormal = sh->addFont("sans", "../example/DroidSerif-Regular.ttf");
	if (fontNormal == fs::INVALID) {
		printf("Could not add font normal.\n");
		return -1;
	}
	fontItalic = sh->addFont("sans-italic", "../example/DroidSerif-Italic.ttf");
	if (fontItalic == fs::INVALID) {
		printf("Could not add font italic.\n");
		return -1;
	}
	fontBold = sh->addFont("sans-bold", "../example/DroidSerif-Bold.ttf");
	if (fontBold == fs::INVALID) {
		printf("Could not add font bold.\n");
		return -1;
	}
	fontJapanese = sh->addFont("sans-jp", "../example/DroidSansJapanese.ttf");
	if (fontJapanese == fs::INVALID) {
		printf("Could not add font japanese.\n");
		return -1;
	}

	while (!glfwWindowShouldClose(window))
	{
		float sx, sy, dx, dy, lh = 0;
		int width, height;
		unsigned int white,black,brown,blue;
		glfwGetFramebufferSize(window, &width, &height);
		// Update and render
		glViewport(0, 0, width, height);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,width,height,0,-1,1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glColor4ub(255,255,255,255);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);

		white = fs::glfonsRGBA(255,255,255,255);
		brown = fs::glfonsRGBA(192,128,0,128);
		blue = fs::glfonsRGBA(0,192,255,255);
		black = fs::glfonsRGBA(0,0,0,255);

		sx = 50; sy = 50;

		dx = sx; dy = sy;

		dash(dx,dy);

		sh->clearState();

		sh->setSize(124.0f);
		sh->setFont(fontNormal);
		sh->vertMetrics(NULL, NULL, &lh);
		dx = sx;
		dy += lh;
		dash(dx,dy);

		sh->setSize(124.0f);
		sh->setFont(fontNormal);
		sh->setColor(white);
		dx = sh->fonsDrawText(dx,dy,"The quick ",NULL);

		sh->setSize(48.0f);
		sh->setFont(fontItalic);
		sh->setColor(brown);
		dx = sh->fonsDrawText(dx,dy,"brown ",NULL);

		sh->setSize(24.0f);
		sh->setFont(fontNormal);
		sh->setColor(white);
		dx = sh->fonsDrawText(dx,dy,"fox ",NULL);

		sh->vertMetrics(NULL, NULL, &lh);
		dx = sx;
		dy += lh*1.2f;
		dash(dx,dy);
		sh->setFont(fontItalic);
		dx = sh->fonsDrawText(dx,dy,"jumps over ",NULL);
		sh->setFont(fontBold);
		dx = sh->fonsDrawText(dx,dy,"the lazy ",NULL);
		sh->setFont(fontNormal);
		dx = sh->fonsDrawText(dx,dy,"dog.",NULL);

		dx = sx;
		dy += lh*1.2f;
		dash(dx,dy);
		sh->setSize(12.0f);
		sh->setFont(fontNormal);
		sh->setColor(blue);
		sh->fonsDrawText(dx,dy,"Now is the time for all good men to come to the aid of the party.",NULL);

		sh->vertMetrics(NULL,NULL,&lh);
		dx = sx;
		dy += lh*1.2f*2;
		dash(dx,dy);
		sh->setSize(18.0f);
		sh->setFont(fontItalic);
		sh->setColor(white);
		sh->fonsDrawText(dx,dy,"Ég get etið gler án þess að meiða mig.",NULL);

		sh->vertMetrics(NULL,NULL,&lh);
		dx = sx;
		dy += lh*1.2f;
		dash(dx,dy);
		sh->setFont(fontJapanese);
		sh->fonsDrawText(dx,dy,"私はガラスを食べられます。それは私を傷つけません。",NULL);

		// Font alignment
		sh->setSize(18.0f);
		sh->setFont(fontNormal);
		sh->setColor(white);

		dx = 50; dy = 350;
		line(dx-10,dy,dx+250,dy);
		sh->setAlign(fs::FONS_ALIGN_LEFT | fs::FONS_ALIGN_TOP);
		dx = sh->fonsDrawText(dx,dy,"Top",NULL);
		dx += 10;
		sh->setAlign(fs::FONS_ALIGN_LEFT | fs::FONS_ALIGN_MIDDLE);
		dx = sh->fonsDrawText(dx,dy,"Middle",NULL);
		dx += 10;
		sh->setAlign(fs::FONS_ALIGN_LEFT | fs::FONS_ALIGN_BASELINE);
		dx = sh->fonsDrawText(dx,dy,"Baseline",NULL);
		dx += 10;
		sh->setAlign(fs::FONS_ALIGN_LEFT | fs::FONS_ALIGN_BOTTOM);
		sh->fonsDrawText(dx,dy,"Bottom",NULL);

		dx = 150; dy = 400;
		line(dx,dy-30,dx,dy+80.0f);
		sh->setAlign(fs::FONS_ALIGN_LEFT | fs::FONS_ALIGN_BASELINE);
		sh->fonsDrawText(dx,dy,"Left",NULL);
		dy += 30;
		sh->setAlign(fs::FONS_ALIGN_CENTER | fs::FONS_ALIGN_BASELINE);
		sh->fonsDrawText(dx,dy,"Center",NULL);
		dy += 30;
		sh->setAlign(fs::FONS_ALIGN_RIGHT | fs::FONS_ALIGN_BASELINE);
		sh->fonsDrawText(dx,dy,"Right",NULL);

		// Blur
		dx = 500; dy = 350;
		sh->setAlign(fs::FONS_ALIGN_LEFT | fs::FONS_ALIGN_BASELINE);

		sh->setSize(60.0f);
		sh->setFont(fontItalic);
		sh->setColor(white);
		sh->setSpacing(5.0f);
		sh->setBlur(10.0f);
		sh->fonsDrawText(dx,dy,"Blurry...",NULL);

		dy += 50.0f;

		sh->setSize(18.0f);
		sh->setFont(fontBold);
		sh->setColor(black);
		sh->setSpacing(0.0f);
		sh->setBlur(3.0f);
		sh->fonsDrawText(dx,dy+2,"DROP THAT SHADOW",NULL);

		sh->setColor(white);
		sh->setBlur(0);
		sh->fonsDrawText(dx,dy,"DROP THAT SHADOW",NULL);

		if (debug)
			sh->drawDebug(800.0, 50.0);


		glEnable(GL_DEPTH_TEST);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfonsDelete(sh);

	glfwTerminate();
	return 0;
}
