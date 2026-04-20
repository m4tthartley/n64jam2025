//
//  Created by Matt Hartley on 19/04/2026.
//  Copyright 2026 GiantJelly. All rights reserved.
//

#ifndef R_RENDER_H
#define R_RENDER_H

#include <core/math.h>


typedef uint32_t color_t;

typedef struct {
	int16_t x, y;
	int16_t w, h;
} vidrect_t;

// 3+3+2+3 = 11*4 = 44
// 3*4 + 3*2 + 2 + 4 = 24
typedef struct {
	vec3_t pos;
	vec3_t normal;
	vec2_t texcoord;
	vec3_t color;
} vertex_t;

typedef struct {
	color_t* texels;
	int width, height;
} texture_t;


#endif
