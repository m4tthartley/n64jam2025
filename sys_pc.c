//
//  Created by Matt Hartley on 28/12/2025.
//  Copyright 2023 GiantJelly. All rights reserved.
//

#include <GL/gl.h>
#define CORE_IMPL
#include <core/core.h>
#include <core/sys.h>
#include <core/video.h>
#include <core/print.h>

#include "r_render_pc.c"


state_t* __state;

state_t* GetState()
{
	return __state;
}

file_data_t* Sys_LoadFile(allocator_t* allocator, char* path)
{
	file_t file = sys_open(path);
	if (!file) {
		return NULL;
	}

	stat_t info = sys_fstat(file);
	file_data_t* result = alloc_memory(allocator, info.size + sizeof(stat_t));
	result->stat = info;
	sys_read(file, 0, result->data, info.size);
	sys_close(file);
	return result;
}

void Init(state_t* state)
{
	state->assetArena = virtual_heap_allocator(MB(100), MB(1));

	state->window = vid_init_window("Linux Window", 320*4, 240*4, 0);

	vid_init_opengl(&state->window);

	state->framebuffer = sys_alloc_memory(320*240*4);

	// GLuint colorBufferTex;
	glGenTextures(1, &state->framebufferTex);
	glBindTexture(GL_TEXTURE_2D, state->framebufferTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 240, 0, GL_RGBA, GL_UNSIGNED_BYTE, state->framebuffer);
}

void Update(state_t* state)
{
	__state = state;
	framebuffer = state->framebuffer;
	window_t* vid = &state->window;
	vid_poll_events(vid);

	if (vid->quit) {
		exit(0);
	}

	
	RenderTestScene();
	

	// glXMakeCurrent(vid->sysDisplay, vid->sysWindow, vid->sysGLContext);

	uint32_t w, h;
	glXQueryDrawable(vid->sysDisplay, vid->sysWindow, GLX_WIDTH, &w);
	glXQueryDrawable(vid->sysDisplay, vid->sysWindow, GLX_HEIGHT, &h);
	// printf("size %i, %i \n", w, h);
	// glDisable(GL_SCISSOR_TEST);
	// glScissor(0, 0, vid->width, vid->height);
	// glViewport(0, 0, vid->width, vid->height);
	glViewport(0, 0, w,  h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, state->framebufferTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 240, 0, GL_RGBA, GL_UNSIGNED_BYTE, state->framebuffer);

	glClearColor(0, 1, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, state->framebufferTex);
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(+1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(+1.0f, +1.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, +1.0f);
	glEnd();

	glXSwapBuffers(vid->sysDisplay, vid->sysWindow);
}

int main()
{
	__state = sys_alloc_memory(sizeof(state_t));
	sys_zero_memory(__state, sizeof(state_t));

	Init(__state);

	dylib_t lib = sys_load_lib("./build/game.so");
	uint64_t modifiedTime = sys_stat("./build/game.so").modified;

	void (*UpdateProc)(state_t* state) = sys_load_lib_sym(lib, "Update");
	state_t** libState = sys_load_lib_sym(lib, "__state");

	for (;;) {
		uint64_t newModifiedTime = sys_stat("./build/game.so").modified;
		if (newModifiedTime != modifiedTime) {
			modifiedTime = newModifiedTime;
			print("Game lib updated \n");

			sys_close_lib(lib);
			lib = sys_load_lib("./build/game.so");
			UpdateProc = sys_load_lib_sym(lib, "Update");

			libState = sys_load_lib_sym(lib, "__state");
			*libState = __state;
		}
		UpdateProc(__state);
	}
}