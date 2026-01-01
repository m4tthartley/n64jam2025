//
//  Created by Matt Hartley on 31/12/2025.
//  Copyright 2023 GiantJelly. All rights reserved.
//

#include "core/sys.h"
#include <core/core.h>
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

uint32_t* framebuffer;
vec3_t __rTranslation = {0};
vec3_t __rRotation = {0};
texture_t* __rActiveTexture = NULL;

void R_SetTranslation(vec3_t translation)
{
	__rTranslation = translation;
}

void R_SetRotation(vec3_t rotation)
{
	__rRotation = rotation;
}

void R_Clear()
{
	sys_zero_memory(framebuffer, 320*240*4);
}

void R_BlitPixel(int x, int y, color_t color)
{
	vidrect_t res = {0, 0, 320, 240};
	if (x >= 0 && x < res.w && y >= 0 && y < res.h) {
		framebuffer[y*res.w + x] = color;
	}
}

void R_DrawLine(float v0x, float v0y, float v1x, float v1y, color_t color)
{
	v0x += __rTranslation.x;
	v0y += __rTranslation.y;
	v1x += __rTranslation.x;
	v1y += __rTranslation.y;

	float v0[] = {v0x, v0y};
	float v1[] = {v1x, v1y};
	float diffx = v1[0] - v0[0];
	float diffy = v1[1] - v0[1];
	bool steep = fabs(diffy) > fabs(diffx);
	int i0 = steep;
	int i1 = !i0;
	if (v0[i0] > v1[i0]) {
		SWAP(v0[0], v1[0]);
		SWAP(v0[1], v1[1]);
	}

	float len = (v1[i0]-v0[i0]);
	float ratio = (v1[i1]-v0[i1]) / len;

	for (int i=0; i<len; ++i) {
		int32_t p[2];
		p[i1] = v0[i1] + ratio*i;
		p[i0] = v0[i0] + i;
		R_BlitPixel(p[0], p[1], color);
	}
}

void R_DrawLineTriangles(vertex_t* verts, int count, color_t color)
{
	assert(count % 3 == 0);

	// R_DrawLine(verts[0].pos.x, verts[0].pos.y, verts[1].pos.x, verts[1].pos.y, color);
	// R_DrawLine(verts[1].pos.x, verts[1].pos.y, verts[2].pos.x, verts[2].pos.y, color);
	// R_DrawLine(verts[2].pos.x, verts[2].pos.y, verts[0].pos.x, verts[0].pos.y, color);
	for (int i=0; i<count; i+=3) {
		for (int v=0; v<3; ++v) {
			vec3_t v0 = verts[i+v].pos;
			vec3_t v1 = verts[i + (v+1)%3].pos;
			R_DrawLine(v0.x, v0.y, v1.x, v1.y, color);
		}
	}
}

vec3_t R_RotateVector(vec3_t v, vec3_t rotation)
{
	// NOTE: Just Z rotation right now
	float zCos = cosf(rotation.z);
	float zSin = sinf(rotation.z);
	vec3_t vector = {
		zCos*v.x + zSin*v.y,
		zCos*v.y - zSin*v.x,
		v.z,
	};

	return vector;
}

float TriangleArea(vec2_t v0, vec2_t v1, vec2_t v2)
{
	return
		((v1.y-v0.y)*(v1.x+v0.x)
		+ (v2.y-v1.y)*(v2.x+v1.x)
		+ (v0.y-v2.y)*(v0.x+v2.x))
		* 0.5f;
}

vec3_t BarycentricCoords(vec2_t coord, vec2_t v0, vec2_t v1, vec2_t v2)
{
	float total = TriangleArea(v0, v1, v2);
	vec3_t result = {
		TriangleArea(coord, v1, v2) / total,
		TriangleArea(coord, v2, v0) / total,
		TriangleArea(coord, v0, v1) / total,
	};

	if (result.x < 0.0f) {
		result.x = 0.0f;
	}
	if (result.y < 0.0f) {
		result.y = 0.0f;
	}
	if (result.z < 0.0f) {
		result.z = 0.0f;
	}

	return result;
}

void R_DrawTriangles(vertex_t* verts, int count, color_t color)
{
	assert(count % 3 == 0);

	// R_DrawLineTriangles(verts, count, 0xFFFFFF);

	vidrect_t res = {0, 0, 320, 240};

	for (int i=0; i<count; i+=3) {
		vertex_t v0 = verts[i+0];
		vertex_t v1 = verts[i+1];
		vertex_t v2 = verts[i+2];
		v0.pos = R_RotateVector(v0.pos, __rRotation);
		v1.pos = R_RotateVector(v1.pos, __rRotation);
		v2.pos = R_RotateVector(v2.pos, __rRotation);
		// vertex_t* v = verts + i;
		
		v0.pos.x += __rTranslation.x;
		v0.pos.y += __rTranslation.y;
		v1.pos.x += __rTranslation.x;
		v1.pos.y += __rTranslation.y;
		v2.pos.x += __rTranslation.x;
		v2.pos.y += __rTranslation.y;

		// Order vertices along y lowest to highest
		if (v1.pos.y < v0.pos.y) SWAP(v0, v1);
		if (v2.pos.y < v1.pos.y) SWAP(v1, v2);
		if (v1.pos.y < v0.pos.y) SWAP(v0, v1);

		// First half
		int line = 0;
		float lineStart = v0.pos.x;
		float lineEnd = v0.pos.x;
		vec2_t longEdgePoint = {v0.pos.x + ((v1.pos.y-v0.pos.y) / (v2.pos.y-v0.pos.y) * (v2.pos.x-v0.pos.x)), v1.pos.y};
		vec2_t startPoint = longEdgePoint;
		vec2_t endPoint = v1.pos.xy;
		if (endPoint.x < startPoint.x) {
			SWAP(startPoint, endPoint);
		}
		float startStep = (startPoint.x-v0.pos.x) / (startPoint.y-v0.pos.y);
		float endStep = (endPoint.x-v0.pos.x) / (endPoint.y-v0.pos.y);
		
		// ++lineEnd.x;
		int lineCount = v1.pos.y-v0.pos.y;
		if ((int)v0.pos.y != (int)v1.pos.y) {
			for (int l=v0.pos.y; l<=v1.pos.y; ++l) {
				float start = lineStart;
				float end = lineEnd;
				// if (startPoint.x > v0.x) {
				// 	start = min(lineStart, startPoint.x);
				// } else {
				// 	start = max(lineStart, startPoint.x);
				// }
				// if (endPoint.x > v0.x) {
				// 	end = min(lineEnd, endPoint.x);
				// } else {
				// 	end = max(lineEnd, endPoint.x);
				// }
				for (int x=start; x<=end; ++x) {
					if (startPoint.x < v0.pos.x && x < startPoint.x) {
						continue;
					}
					if (endPoint.x > v0.pos.x && x > endPoint.x) {
						continue;
					}

					vec3_t triCoords = BarycentricCoords(vec2(x, l), v0.pos.xy, v1.pos.xy, v2.pos.xy);
					vec3_t color3 = {
						v0.color.r*triCoords.f[0] + v1.color.r*triCoords.f[1] + v2.color.r*triCoords.f[2],
						v0.color.g*triCoords.f[0] + v1.color.g*triCoords.f[1] + v2.color.g*triCoords.f[2],
						v0.color.b*triCoords.f[0] + v1.color.b*triCoords.f[1] + v2.color.b*triCoords.f[2],
					};

					uint32_t color =
						((uint32_t)(color3.r*255.0f)<<0) |
						((uint32_t)(color3.g*255.0f)<<8) |
						((uint32_t)(color3.b*255.0f)<<16);

					framebuffer[l*res.w + x] = color;
				}

				lineStart += startStep;
				lineEnd += endStep;
			}
		}

		// Second half
		lineStart = longEdgePoint.x;
		lineEnd = v1.pos.x;
		startStep = (v2.pos.x-longEdgePoint.x) / (v2.pos.y-longEdgePoint.y);
		endStep = (v2.pos.x-v1.pos.x) / (v2.pos.y-v1.pos.y);
		if (v1.pos.x < longEdgePoint.x) {
			SWAP(lineStart, lineEnd);
			SWAP(startStep, endStep);
		}

		// if ((int)v2.y != (int)v1.y) {
			for (int l=v1.pos.y; l<=v2.pos.y; ++l) {
				for (int x=lineStart; x<=lineEnd; ++x) {
					vec3_t triCoords = BarycentricCoords(vec2(x, l), v0.pos.xy, v1.pos.xy, v2.pos.xy);
					vec3_t color3 = {
						v0.color.r*triCoords.f[0] + v1.color.r*triCoords.f[1] + v2.color.r*triCoords.f[2],
						v0.color.g*triCoords.f[0] + v1.color.g*triCoords.f[1] + v2.color.g*triCoords.f[2],
						v0.color.b*triCoords.f[0] + v1.color.b*triCoords.f[1] + v2.color.b*triCoords.f[2],
					};

					uint32_t color =
						((uint32_t)(color3.r*255.0f)<<0) |
						((uint32_t)(color3.g*255.0f)<<8) |
						((uint32_t)(color3.b*255.0f)<<16);

					framebuffer[l*res.w + x] = color;
				}

				lineStart += startStep;
				lineEnd += endStep;
			}
		// }

		R_BlitPixel(v0.pos.x, v0.pos.y, 0xFFFFFF);
		R_BlitPixel(v1.pos.x, v1.pos.y, 0xFFFFFF);
		R_BlitPixel(v2.pos.x, v2.pos.y, 0xFFFFFF);
	}
}

void RenderTestScene()
{
	static texture_t texture;
	if (!__rActiveTexture) {
		__rActiveTexture = &texture;
		texture.width = 32;
		texture.height = 32;
		texture.texels = sys_alloc_memory(texture.width*texture.height*sizeof(color_t));
		for (int i=0; i<texture.width*texture.height; ++i) {
			texture.texels[i] = rand() & 0xFF;
		}
	}

	R_Clear();

	// for (int i=0; i<320*240; ++i) {
		static int i = 0;
		int r = rand() & 0xFF;
		int g = rand() & 0xFF;
		int b = rand() & 0xFF;
		framebuffer[i] = (0xFF<<24) | (r<<16) | (g<<8) | (b<<0);
		++i;
		i %= (320*240);
	// }

	static float x = 0.0f;
	x += 0.01f;
	R_SetTranslation(vec3(160, 120, 0));
	R_DrawLine(cosf(x)*100, sinf(x)*100, cosf(x)*-100, sinf(x)*-100, 0x0000FF);
	R_SetTranslation(vec3(0, 0, 0));
	R_DrawLine(120, 80, 220, 150, 0xFF0000);

	vertex_t verts[] = {
		{vec3(-10.0f, -10.0f, 0)},
		{vec3(10.0f, 0.0f, 0)},
		{vec3(0.0f, 10.0f, 0)},

		{vec3(-5.0f, -5.0f, 0)},
		{vec3(5.0f, 0.0f, 0)},
		{vec3(0.0f, 5.0f, 0)},
	};

	for (int i=0; i<array_size(verts); ++i) {
		vec3_t v = verts[i].pos;
		verts[i].pos = vec3(cosf(x)*v.x + sinf(x)*v.y, cosf(x)*v.y - sinf(x)*v.x, 0);
		// verts[i].pos.x += 20;
		// verts[i].pos.y += 20;
	}

	R_SetTranslation(vec3(50, 50, 0));
	R_DrawLineTriangles(verts, 6, 0x00FF00);

	vertex_t verts2[] = {
		{vec3(-28.0f, -20.0f, 0), .texcoord=vec2(0, 0), .color=vec3(1, 0, 0)},
		{vec3(+48.0f, +0.0f, 0), .texcoord=vec2(1, 0), .color=vec3(0, 1, 0)},
		{vec3(+12.0f, +28.0f, 0), .texcoord=vec2(1, 1), .color=vec3(0, 0, 1)},
	};
	// vertex_t verts2[] = {
	// 	{vec3(-28.0f, -5.0f, 0)},
	// 	{vec3(+48.0f, +0.0f, 0)},
	// 	{vec3(+12.0f, +2.0f, 0)},
	// };

	R_SetTranslation(vec3(160, 120, 0));
	R_SetRotation(vec3(0, 0, x));
	R_DrawTriangles(verts2, 3, 0xFF8888);
}
