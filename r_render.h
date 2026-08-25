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
typedef struct { // TODO: Pack tight
	vec3_t pos;
	vec3_t normal;
	vec3_t color;
	vec2_t texcoord;
} mvertex_t;

typedef struct { // TODO: Pack tight
	vec4_t pos;
	vec2_t texcoord;
	vec3_t normal;
	vec3_t color;
} vertex_t;

typedef struct {
	color_t* texels;
	int width, height;
} texture_t;

// I'm currently thinking that each mesh will be part of a model
// with a different texture
// But it might end up that a mesh is each part of the skeleton
typedef struct {
	mvertex_t* vertices;
	uint32_t vertexCount;
	texture_t texture;
} mesh_t;

typedef struct {
	mesh_t* meshes;
	uint32_t meshCount;
} model_t;

typedef enum {
	R_PROJECTION_NULL,
	R_PERSPECTIVE = 1,
	R_ORTHO,
} projection_type_t;

typedef struct {
	float xFactor;
	float yFactor;
	float zFactor;
	float zOffset;
} perspective_factors_t;


extern inline vertex_t MVertexToVertex(mvertex_t v)
{
	vertex_t result = {
		.pos = vec4f3(v.pos, 1),
		.texcoord = v.texcoord,
		.normal = v.normal,
		.color = v.color,
	};

	return result;
}


color_t Color32(float r, float g, float b, float a);
color_t Color32FromVec3(vec3_t v);
vec4_t Color32ToFloat(color_t c);
uint32_t MixColor32(color_t a, color_t b);
float TriangleArea(vec2_t v0, vec2_t v1, vec2_t v2);
vec3_t BarycentricCoords(vec2_t coord, vec2_t v0, vec2_t v1, vec2_t v2);
vertex_t LerpTriVetices(vec2_t coord, vertex_t v0, vertex_t v1, vertex_t v2);
vec4_t ClipSpaceToFramebufferSpace(vec4_t v);

void R_RasterizePoint(vec4_t pos, color_t color);
void R_RasterizeLine(vec4_t v0, vec4_t v1, color_t color);


#endif
