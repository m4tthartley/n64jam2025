//
//  Created by Matt Hartley on 21/04/2026.
//  Copyright 2026 GiantJelly. All rights reserved.
//

#ifndef SYS_PC_H
#define SYS_PC_H

#include <GL/gl.h>
#include <core/core.h>
#include <core/video.h>


typedef struct {
	window_t window;
	uint32_t* framebuffer;
	GLuint framebufferTex;

	allocator_t assetArena;
} state_t;

state_t* GetState();
file_data_t* Sys_LoadFile(allocator_t* allocator, char* path);


#endif
