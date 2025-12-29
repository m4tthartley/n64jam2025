//
//  Created by Matt Hartley on 28/12/2025.
//  Copyright 2023 GiantJelly. All rights reserved.
//

#include <GL/gl.h>
#define CORE_IMPL
#include <core/sys.h>
#include <core/video.h>

int main()
{
	// sys_window_t window;
	// sys_init_window(&window, "Linux Window", 800, 600, 0);

	// sys_init_opengl(&window);

	int x = 0;
	window_t window = vid_init_window("Linux Window", 800, 600, 0);

	vid_init_opengl(&window);

	uint32_t* framebuffer = sys_alloc_memory(320*240*4);

	GLuint colorBufferTex;
	glGenTextures(1, &colorBufferTex);
	// glTexSubImage2D(colorBufferTex, )
	glBindTexture(GL_TEXTURE_2D, colorBufferTex);
	// glTexSubImage2D(colorBufferTex, 0, 0, 0, 320, 240, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 240, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);

	for (int i=0; i<320*240; ++i) {
		int r = rand() & 0xFF;
		int g = rand() & 0xFF;
		int b = rand() & 0xFF;
		framebuffer[i] = (0xFF<<24) | (r<<16) | (g<<8) | (b<<0);
	}

	for (;;) {
		vid_poll_events(&window);

		if (window.quit) {
			exit(0);
		}

		// glViewport(0, 0, window.width, window.height);

		glBindTexture(GL_TEXTURE_2D, colorBufferTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 240, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);

		glClearColor(1, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, colorBufferTex);
		glColor4f(1, 1, 1, 1);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f, -0.5f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(+0.5f, -0.5f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(+0.5f, +0.5f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.5f, +0.5f);
		glEnd();

		glXSwapBuffers(window.sysDisplay, window.sysWindow);
	}
}