//
//  Created by Matt Hartley on 31/12/2025.
//  Copyright 2023 GiantJelly. All rights reserved.
//

#include <core/core.h>
#include <core/math.h>
#include <core/time.h>
#include <core/bmp.h>

#include "sys_pc.h"
#include "r_render.h"
#include "r_clip.c"
#include "r_rasterize_scanline.c"
#include "r_rasterize_bbox.c"


uint32_t* framebuffer;
float* depthbuffer;
vec3_t __rTranslation = {0};
vec3_t __rRotation = {0};
vec3_t __rScale = {1, 1, 1};
texture_t* __rActiveTexture = NULL;
projection_type_t __rProjectionType = R_PROJECTION_NULL;
perspective_factors_t __rPerspectiveFactors = {0};
bool_t __rScanlineRasterization = FALSE;


color_t Color32(float r, float g, float b, float a)
{
	uint32_t color =
		(uint32_t)(r*255.0f) |
		(uint32_t)(g*255.0f)<<8 |
		(uint32_t)(b*255.0f)<<16 |
		(uint32_t)(a*255.0f)<<24;
	return color;
}

color_t Color32FromVec3(vec3_t v)
{
	uint32_t color =
		(uint32_t)(v.r*255.0f) |
		(uint32_t)(v.g*255.0f)<<8 |
		(uint32_t)(v.b*255.0f)<<16;
	return color;
}

vec4_t Color32ToFloat(color_t c)
{
	vec4_t color = {
		(c&0xFF)/255.0f,
		((c>>8)&0xFF)/255.0f,
		((c>>16)&0xFF)/255.0f,
		((c>>24)&0xFF)/255.0f,
	};

	return color;
}

uint32_t MixColor32(color_t a, color_t b)
{
	vec4_t af = Color32ToFloat(a);
	vec4_t bf = Color32ToFloat(b);
	vec4_t c = mul4(af, bf);
	return Color32(c.r, c.g, c.b, c.a);
}

void R_SetTranslation(vec3_t translation)
{
	__rTranslation = translation;
}

void R_SetRotation(vec3_t rotation)
{
	__rRotation = rotation;
}

void R_SetScale(vec3_t scale)
{
	__rScale = scale;
}

void R_Texture(texture_t* tex)
{
	__rActiveTexture = tex;
}

void R_Projection(projection_type_t type)
{
	__rProjectionType = type;
}

void R_PerspectiveProjection(float fov, float aspect, float near, float far)
{
	R_Projection(R_PERSPECTIVE);

	float a = aspect;
	float s = 1.0f / tanf((fov/180.0f*PI) / 2.0f);
	float n = near;
	float f = far;

	__rPerspectiveFactors = (perspective_factors_t){
		.xFactor = s / a,
		.yFactor = s,
		.zFactor = (f+n)/(n-f),
		.zOffset = ((2*f*n)/(n-f)) * 1,
	};
}

vertex_t R_PerspectiveTransform(mvertex_t in)
{
	vertex_t out;
	out.texcoord = in.texcoord;
	out.normal = in.normal;
	out.color = in.color;

	perspective_factors_t p = __rPerspectiveFactors;

	out.pos.x = in.pos.x * p.xFactor;
	out.pos.y = in.pos.y * p.yFactor;
	out.pos.z = in.pos.z * p.zFactor + p.zOffset;
	out.pos.w = -in.pos.z;

	return out;
}

void R_Clear()
{
	state_t* state = GetState();
	sys_zero_memory(state->framebuffer, 320*240*4);
	// sys_zero_memory(state->depthbuffer, 320*240*sizeof(float));
	for (int idx=0; idx<320*240; ++idx) {
		depthbuffer[idx] = 100.0f;
	}
}

vec3_t R_RotateVector3(vec3_t v, vec3_t rotation)
{
	float xCos = cosf(rotation.x);
	float xSin = sinf(rotation.x);
	v = (vec3_t){
		v.x,
		xCos*v.y + xSin*v.z,
		xCos*v.z - xSin*v.y,
	};

	float yCos = cosf(rotation.y);
	float ySin = sinf(rotation.y);
	v = (vec3_t){
		yCos*v.x + ySin*v.z,
		v.y,
		yCos*v.z - ySin*v.x,
	};

	// NOTE: Just Z rotation right now
	float zCos = cosf(rotation.z);
	float zSin = sinf(rotation.z);
	v = (vec3_t){
		zCos*v.x + zSin*v.y,
		zCos*v.y - zSin*v.x,
		v.z,
	};

	return v;
}

float TriangleArea(vec2_t v0, vec2_t v1, vec2_t v2)
{
	// This gives a signed area depending on the order
	float result =
		((v1.y-v0.y)*(v1.x+v0.x)
		+ (v2.y-v1.y)*(v2.x+v1.x)
		+ (v0.y-v2.y)*(v0.x+v2.x))
		* 0.5f;

	// if (result < 0.0f) {
	// 	return -result;
	// }
	return (result);
}

vec3_t BarycentricCoords(vec2_t coord, vec2_t v0, vec2_t v1, vec2_t v2)
{
	float total = TriangleArea(v0, v1, v2);
	// total = fabs(total);

	if (total < 1.0f) {
		return vec3f(0.0f);
	}

	vec3_t result = {
		TriangleArea(coord, v1, v2) / total,
		TriangleArea(coord, v2, v0) / total,
		TriangleArea(coord, v0, v1) / total,
	};

	// if (isnan(result.z)) {
	// 	int x =0;
	// }

	// if (result.x < 0.0f) {
	// 	result.x = 0.0f;
	// }
	// if (result.x >= 1.0f) {
	// 	result.x = 0.999f;
	// }
	// if (result.y < 0.0f) {
	// 	result.y = 0.0f;
	// }
	// if (result.y >= 1.0f) {
	// 	result.y = 0.999f;
	// }
	// if (result.z < 0.0f) {
	// 	result.z = 0.0f;
	// }
	// if (result.z >= 1.0f) {
	// 	result.z = 0.999f;
	// }

	return result;
}

vertex_t LerpTriVetices(vec2_t coord, vertex_t v0, vertex_t v1, vertex_t v2)
{
	vec3_t t = BarycentricCoords(vec2(coord.x, coord.y), v0.pos.xy, v1.pos.xy, v2.pos.xy);
	vertex_t v;
	// v.pos = vec4f2(coord, 0, 1);

	vec4_t pos = {
		v0.pos.x*t.x + v1.pos.x*t.y + v2.pos.x*t.z,
		v0.pos.y*t.x + v1.pos.y*t.y + v2.pos.y*t.z,
		v0.pos.z*t.x + v1.pos.z*t.y + v2.pos.z*t.z,
		v0.pos.w*t.x + v1.pos.w*t.y + v2.pos.w*t.z,
	};

	vec3_t color3 = {
		v0.color.r*t.x + v1.color.r*t.y + v2.color.r*t.z,
		v0.color.g*t.x + v1.color.g*t.y + v2.color.g*t.z,
		v0.color.b*t.x + v1.color.b*t.y + v2.color.b*t.z,
	};

	vec2_t texcoord = {
		// clamp(v0.texcoord.x*t.x + v1.texcoord.x*t.y + v2.texcoord.x*t.z, 0.001f, 0.999f),
		// clamp(v0.texcoord.y*t.x + v1.texcoord.y*t.y + v2.texcoord.y*t.z, 0.001f, 0.999f),
		v0.texcoord.x*t.x + v1.texcoord.x*t.y + v2.texcoord.x*t.z,
		v0.texcoord.y*t.x + v1.texcoord.y*t.y + v2.texcoord.y*t.z,
	};

	// if (isnan(texcoord.x) || isnan(texcoord.y)) {
	// 	int x = 0;
	// 	print("NOOOOOOOOOOOOOOO");
	// }

	// if (isnan(pos.x) || isnan(pos.y)) {
	// 	int x = 0;
	// 	print("NOOOOOOOOOOOOOOO");
	// }

	v.pos = pos;
	v.color = color3;
	v.texcoord = texcoord;

	return v;
}

vec4_t ClipSpaceToFramebufferSpace(vec4_t v)
{
	vidrect_t res = {0, 0, 320, 240};

	// v = mul3(v, vec3f(0.5f));
	vec4_t result = {
		(v.x + 1.0f) * 160.0f,
		(v.y + 1.0f) * 120.0f,
		v.z,
		v.w,
	};

	return result;
}

void R_BlitPixel(int x, int y, color_t color)
{
	vidrect_t res = {0, 0, 320, 240};
	if (x >= 0 && x < res.w && y >= 0 && y < res.h) {
		framebuffer[y*res.w + x] = color;
	}
}

/*
	The Rasterize functions take NDCs
*/
void R_RasterizePoint(vec4_t pos, color_t color)
{
	vidrect_t res = {0, 0, 320, 240};

	vec4_t coords = ClipSpaceToFramebufferSpace(pos);
	int x = coords.x;
	int y = coords.y;

	if (x >= res.x && x < res.w && y >= res.y && y < res.h) {
		R_BlitPixel(coords.x, coords.y, color);
	}
}

void R_RasterizeLine(vec4_t v0, vec4_t v1, color_t color)
{
	vidrect_t res = {0, 0, 320, 240};

	// Divide by W
	v0.xyz = div3(v0.xyz, vec3f(v0.w));
	v1.xyz = div3(v1.xyz, vec3f(v1.w));

	vec4_t vec0 = ClipSpaceToFramebufferSpace(v0);
	vec4_t vec1 = ClipSpaceToFramebufferSpace(v1);
	// int x0 = coords0.x;
	// int y0 = coords0.y;
	// int x1 = coords1.x;
	// int y1 = coords1.y;

	// if (x >= res.x && x < res.w && y >= res.y && y < res.h) {
		// R_DrawLine(v0.x, v0.y, v1.x, v1.y, color);
	// }

	float v0x = vec0.x;
	float v0y = vec0.y;
	float v1x = vec1.x;
	float v1y = vec1.y;

	v0x += __rTranslation.x;
	v0y += __rTranslation.y;
	v1x += __rTranslation.x;
	v1y += __rTranslation.y;

	float v0xy[] = {v0x, v0y};
	float v1xy[] = {v1x, v1y};
	float diffx = v1xy[0] - v0xy[0];
	float diffy = v1xy[1] - v0xy[1];
	bool steep = fabs(diffy) > fabs(diffx);
	int i0 = steep;
	int i1 = !i0;
	if (v0xy[i0] > v1xy[i0]) {
		SWAP(v0xy[0], v1xy[0]);
		SWAP(v0xy[1], v1xy[1]);
	}

	float len = (v1xy[i0]-v0xy[i0]);
	float ratio = (v1xy[i1]-v0xy[i1]) / len;

	for (int i=0; i<len; ++i) {
		int32_t p[2];
		p[i1] = v0xy[i1] + ratio*i;
		p[i0] = v0xy[i0] + i;
		R_BlitPixel(p[0], p[1], color);
	}
}

// void R_DrawLine(float v0x, float v0y, float v1x, float v1y, color_t color)
// {
	
// }

// void R_DrawLineTriangles(mvertex_t* verts, int count, color_t color)
// {
// 	assert(count % 3 == 0);

// 	// R_DrawLine(verts[0].pos.x, verts[0].pos.y, verts[1].pos.x, verts[1].pos.y, color);
// 	// R_DrawLine(verts[1].pos.x, verts[1].pos.y, verts[2].pos.x, verts[2].pos.y, color);
// 	// R_DrawLine(verts[2].pos.x, verts[2].pos.y, verts[0].pos.x, verts[0].pos.y, color);
// 	for (int i=0; i<count; i+=3) {
// 		for (int v=0; v<3; ++v) {
// 			vec3_t v0 = verts[i+v].pos;
// 			vec3_t v1 = verts[i + (v+1)%3].pos;
// 			R_DrawLine(v0.x, v0.y, v1.x, v1.y, color);
// 		}
// 	}
// }

vertex_t R_TransformAndProject(mvertex_t in)
{
	in.pos = R_RotateVector3(in.pos, __rRotation);
	in.pos = mul3(in.pos, __rScale);
	in.pos = add3(in.pos, __rTranslation);

	vertex_t out;

	switch (__rProjectionType) {
		case R_PERSPECTIVE:
			out = R_PerspectiveTransform(in);
			break;
		default:
			out = MVertexToVertex(in);
			break;
	}

	return out;
}

vec3_t R_LightVertex(mvertex_t v)
{
	float ambient = 0.1f;
	vec3_t sunDir = normalize3(vec3(1, 1, 1));
	float sunLight = max(dot3(v.normal, sunDir), 0.0f);
	float light = ambient + sunLight;
	return vec3f(min(light, 1.0f));
}

void R_DrawTriangle(mvertex_t in0, mvertex_t in1, mvertex_t in2, color_t color)
{
	// vertex_t v0 = MVertexToVertex(in0);
	// vertex_t v1 = MVertexToVertex(in1);
	// vertex_t v2 = MVertexToVertex(in2);

	// World Transform
	in0.pos = R_RotateVector3(in0.pos, __rRotation);
	in1.pos = R_RotateVector3(in1.pos, __rRotation);
	in2.pos = R_RotateVector3(in2.pos, __rRotation);
	in0.normal = R_RotateVector3(in0.normal, __rRotation);
	in1.normal = R_RotateVector3(in1.normal, __rRotation);
	in2.normal = R_RotateVector3(in2.normal, __rRotation);
	// vertex_t* v = verts + i;

	in0.pos = mul3(in0.pos, __rScale);
	in1.pos = mul3(in1.pos, __rScale);
	in2.pos = mul3(in2.pos, __rScale);
	
	in0.pos = add3(in0.pos, __rTranslation);
	in1.pos = add3(in1.pos, __rTranslation);
	in2.pos = add3(in2.pos, __rTranslation);

	// Lighting
	// Gouraud shading
	in0.color = mul3(in0.color, R_LightVertex(in0));
	in1.color = mul3(in1.color, R_LightVertex(in1));
	in2.color = mul3(in2.color, R_LightVertex(in2));

	// Projection transform
	vertex_t v0;
	vertex_t v1;
	vertex_t v2;

	switch (__rProjectionType) {
		case R_PERSPECTIVE:
			v0 = R_PerspectiveTransform(in0);
			v1 = R_PerspectiveTransform(in1);
			v2 = R_PerspectiveTransform(in2);
			break;
		default:
			v0 = MVertexToVertex(in0);
			v1 = MVertexToVertex(in1);
			v2 = MVertexToVertex(in2);
			break;
	}

	// v0.pos.w = 1.0f;
	// v1.pos.w = 1.0f;
	// v2.pos.w = 1.0f;

	// Screen clipping
	polygon_t poly = {.vertices={v0, v1, v2}, .count=3};
	poly = R_ClipWithPlane(poly, PLANE_X0);
	poly = R_ClipWithPlane(poly, PLANE_X1);
	poly = R_ClipWithPlane(poly, PLANE_Y0);
	poly = R_ClipWithPlane(poly, PLANE_Y1);
	poly = R_ClipWithPlane(poly, PLANE_Z0);
	poly = R_ClipWithPlane(poly, PLANE_Z1);

	// Rasterize loop
	for (int vi=0; vi<(poly.count-2); ++vi) {
		vertex_t t0 = poly.vertices[0];
		vertex_t t1 = poly.vertices[vi+1];
		vertex_t t2 = poly.vertices[vi+2];

		if (__rScanlineRasterization) {
			R_RasterizeScanlineTriangle(t0, t1, t2, color);
		} else {
			R_RasterizeBBoxTriangle(t0, t1, t2, color);
		}

		// R_RasterizePoint(vec3(t0.pos.x, t0.pos.y, 0), 0xFFFFFF);
		// R_RasterizePoint(vec3(t1.pos.x, t1.pos.y, 0), 0xFFFFFF);
		// R_RasterizePoint(vec3(t2.pos.x, t2.pos.y, 0), 0xFFFFFF);
	}

	// R_RasterizeTriangle(v0, v1, v2, color);
}

void R_DrawLineTriangle(mvertex_t in0, mvertex_t in1, mvertex_t in2, color_t color)
{
	vertex_t v0 = R_TransformAndProject(in0);
	vertex_t v1 = R_TransformAndProject(in1);
	vertex_t v2 = R_TransformAndProject(in2);

	polygon_t poly = {.vertices={v0, v1, v2}, .count=3};
	poly = R_ClipWithPlane(poly, PLANE_X0);
	poly = R_ClipWithPlane(poly, PLANE_X1);
	poly = R_ClipWithPlane(poly, PLANE_Y0);
	poly = R_ClipWithPlane(poly, PLANE_Y1);
	poly = R_ClipWithPlane(poly, PLANE_Z0);
	poly = R_ClipWithPlane(poly, PLANE_Z1);

	for (int vi=0; vi<(poly.count-2); ++vi) {
		vertex_t t0 = poly.vertices[0];
		vertex_t t1 = poly.vertices[vi+1];
		vertex_t t2 = poly.vertices[vi+2];

		// R_RasterizeTriangle(t0, t1, t2, color);
		R_RasterizeLine(t0.pos, t1.pos, color);
		R_RasterizeLine(t1.pos, t2.pos, color);
		R_RasterizeLine(t0.pos, t2.pos, color);
	}
}

void R_DrawTriangles(mvertex_t* verts, int count, color_t color)
{
	assert(count % 3 == 0);

	// R_DrawLineTriangles(verts, count, 0xFFFFFF);

	for (int i=0; i<count; i+=3) {
		mvertex_t v0 = verts[i+0];
		mvertex_t v1 = verts[i+1];
		mvertex_t v2 = verts[i+2];

		R_DrawTriangle(v0, v1, v2, color);
	}
}

void R_DrawLineTriangles(mvertex_t* verts, int count, color_t color)
{
	assert(count % 3 == 0);

	// R_DrawLine(verts[0].pos.x, verts[0].pos.y, verts[1].pos.x, verts[1].pos.y, color);
	// R_DrawLine(verts[1].pos.x, verts[1].pos.y, verts[2].pos.x, verts[2].pos.y, color);
	// R_DrawLine(verts[2].pos.x, verts[2].pos.y, verts[0].pos.x, verts[0].pos.y, color);
	// for (int i=0; i<count; i+=3) {
	// 	for (int v=0; v<3; ++v) {
	// 		vec3_t v0 = verts[i+v].pos;
	// 		vec3_t v1 = verts[i + (v+1)%3].pos;
	// 		R_DrawLine(v0.x, v0.y, v1.x, v1.y, color);
	// 	}
	// }

	for (int i=0; i<count; i+=3) {
		mvertex_t v0 = verts[i+0];
		mvertex_t v1 = verts[i+1];
		mvertex_t v2 = verts[i+2];

		R_DrawLineTriangle(v0, v1, v2, color);
	}
}

void R_DrawQuads(mvertex_t* verts, int count, color_t color)
{
	assert(count % 4 == 0);

	for (int i=0; i<count; i+=4) {
		mvertex_t v0 = verts[i+0];
		mvertex_t v1 = verts[i+1];
		mvertex_t v2 = verts[i+2];
		mvertex_t v3 = verts[i+3];

		R_DrawTriangle(v0, v1, v2, color);
		R_DrawTriangle(v2, v3, v0, color);
	}
}

void R_DrawModel(model_t model, color_t color)
{
	for (int mi=0; mi<1; ++mi) {
		R_Texture(NULL);///&model.meshes[mi].texture);
		R_DrawTriangles(model.meshes[mi].vertices, model.meshes[mi].vertexCount, color);
	}
}

void R_DrawLineModel(model_t model, color_t color)
{
	for (int mi=0; mi<model.meshCount; ++mi) {
		R_DrawLineTriangles(model.meshes[mi].vertices, model.meshes[mi].vertexCount, color);
	}
}

texture_t testTexture;

void R_Init()
{
	
}

void RenderTestScene()
{
	static double prevTime;
	double time = time_get_ms();
	float delta = clamp((time - prevTime) / 1000.0f, 0, 0.5f);
	prevTime = time;

	// static texture_t texture;
	// if (!__rActiveTexture) {
	// 	__rActiveTexture = &texture;
	// 	texture.width = 32;
	// 	texture.height = 32;
	// 	texture.texels = sys_alloc_memory(texture.width*texture.height*sizeof(color_t));
	// 	// for (int i=0; i<texture.width*texture.height; ++i) {
	// 	// 	texture.texels[i] = rand() & 0xFF;
	// 	// }
	// 	for (int y=0; y<32; ++y) {
	// 		for (int x=0; x<32; ++x) {
	// 			// float r = rand() & 0xFF;
	// 			// texture.texels[y*32+x] = (int)(r*((float)y/32)) | (int)(r*(1.0f-(float)y/32))<<8;

	// 			// texture.texels[y*32+x] = 255*(y&1) | (255<<8)*(!(y&1)) | (255<<16)*(x&4);

	// 			texture.texels[y*32+x] = Color32((float)(31-x)/32, (float)(x)/32, (float)(y)/32, 0);
	// 		}
	// 	}
	// }

	if (!testTexture.texels) {
		testTexture = R_LoadTexture("assets/metal2.bmp");
	}

	R_Texture(&testTexture);

	R_Clear();

	for (int y=0; y<32; ++y) {
		for (int x=0; x<32; ++x) {
			framebuffer[(240-10-y) * 320 + (10+x)] = testTexture.texels[y*32+x];
		}
	}

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
	x += delta * 1.0f;
	R_SetTranslation(vec3(160, 120, 0));
	// R_DrawLine(cosf(x)*100, sinf(x)*100, cosf(x)*-100, sinf(x)*-100, 0x0000FF);
	R_SetTranslation(vec3(0, 0, 0));
	// R_DrawLine(120, 80, 220, 150, 0xFF0000);

	mvertex_t verts[] = {
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

	R_SetTranslation(vec3(30, 30, 0));
	R_DrawLineTriangles(verts, 6, 0x00FFFF);

	mvertex_t verts2[] = {
		// {vec3(-28.0f, -20.0f, 0), .texcoord=vec2(0, 0), .color=vec3(1, 0, 0)},
		// {vec3(+48.0f, +0.0f, 0), .texcoord=vec2(1, 0), .color=vec3(0, 1, 0)},
		// {vec3(+12.0f, +28.0f, 0), .texcoord=vec2(1, 1), .color=vec3(0, 0, 1)},
		{vec3(-0.5f, -0.5f, 0), .texcoord=vec2(0, 0), .color=vec3(1, 1, 1)},
		{vec3(+0.5f, -0.5f, 0), .texcoord=vec2(1, 0), .color=vec3(1, 1, 1)},
		{vec3(+0.0f, +0.5f, 0), .texcoord=vec2(1, 1), .color=vec3(1, 1, 1)},
	};
	// vertex_t verts2[] = {
	// 	{vec3(-28.0f, -5.0f, 0)},
	// 	{vec3(+48.0f, +0.0f, 0)},
	// 	{vec3(+12.0f, +2.0f, 0)},
	// };

	R_SetTranslation(vec3(0.5f, 0.0f, 0));
	R_SetRotation(vec3(0, 0, x));
	R_SetScale(vec3f(2.0f));
	R_DrawTriangles(verts2, 3, 0xFF8888);

	// R_BlitPixel(160, 120, Color32(1, 0, 1, 1));
}

void Render3DTestScene()
{
	static double prevTime;
	double time = time_get_ms();
	float delta = clamp((time - prevTime) / 1000.0f, 0, 0.5f);
	prevTime = time;

	if (!testTexture.texels) {
		testTexture = R_LoadTexture("assets/metal2.bmp");
	}

	R_Texture(&testTexture);

	R_Clear();

	static float x = 0.0f;
	x += delta * 1.0f;

	mvertex_t verts2[] = {
		{vec3(-0.5f, -0.5f, +0.5f), .texcoord=vec2(0, 0), /*.color=vec3(1, 0, 0)},*/ .color=vec3(1, 1, 1)},
		{vec3(+0.5f, -0.5f, +0.5f), .texcoord=vec2(1, 0), /*.color=vec3(0, 1, 0)},*/ .color=vec3(1, 1, 1)},
		{vec3(+0.5f, +0.5f, +0.5f), .texcoord=vec2(1, 1), /*.color=vec3(0, 0, 1)},*/ .color=vec3(1, 1, 1)},
		{vec3(-0.5f, +0.5f, +0.5f), .texcoord=vec2(0, 1), /*.color=vec3(1, 0, 1)},*/ .color=vec3(1, 1, 1)},

		{vec3(+0.5f, -0.5f, +0.5f), .texcoord=vec2(0, 0), /*.color=vec3(1, 0, 0)},*/ .color=vec3(1, 1, 1)},
		{vec3(+0.5f, -0.5f, -0.5f), .texcoord=vec2(1, 0), /*.color=vec3(0, 1, 0)},*/ .color=vec3(1, 1, 1)},
		{vec3(+0.5f, +0.5f, -0.5f), .texcoord=vec2(1, 1), /*.color=vec3(0, 0, 1)},*/ .color=vec3(1, 1, 1)},
		{vec3(+0.5f, +0.5f, +0.5f), .texcoord=vec2(0, 1), /*.color=vec3(1, 0, 1)},*/ .color=vec3(1, 1, 1)},

		{vec3(-0.5f, -0.5f, -0.5f), .texcoord=vec2(0, 0), /*.color=vec3(1, 0, 0)},*/ .color=vec3(1, 1, 1)},
		{vec3(-0.5f, -0.5f, +0.5f), .texcoord=vec2(1, 0), /*.color=vec3(0, 1, 0)},*/ .color=vec3(1, 1, 1)},
		{vec3(-0.5f, +0.5f, +0.5f), .texcoord=vec2(1, 1), /*.color=vec3(0, 0, 1)},*/ .color=vec3(1, 1, 1)},
		{vec3(-0.5f, +0.5f, -0.5f), .texcoord=vec2(0, 1), /*.color=vec3(1, 0, 1)},*/ .color=vec3(1, 1, 1)},
	};

	// R_Projection(R_PERSPECTIVE);
	R_PerspectiveProjection(70, 320.0f/240.0f, 0.1f, 100.0f);

	R_SetTranslation(vec3(0.0f, 0.0, -1.0f - (sinf(x) * 0.5f)));
	R_SetRotation(vec3(0, x, 0));
	// R_SetRotation(vec3(0, -0.5, 0));
	R_SetScale(vec3f(0.5f));
	R_DrawQuads(verts2, 12, 0xFF8888);
	// R_DrawTriangles(verts2, 3, 0xFF8888);
}

void Render3DModelTestScene()
{
	state_t* state = GetState();

	static double prevTime;
	double time = time_get_ms();
	float delta = clamp((time - prevTime) / 1000.0f, 0, 0.5f);
	prevTime = time;

	if (!testTexture.texels) {
		testTexture = R_LoadTexture("assets/metal2.bmp");
	}

	R_Texture(&testTexture);

	R_Clear();

	static float x = 0.0f;
	x += delta * 1.0f;

	mvertex_t verts2[] = {
		{vec3(-0.5f, -0.5f, +0.5f), .texcoord=vec2(0, 0), /*.color=vec3(1, 0, 0)},*/ .color=vec3(1, 1, 1), .normal=vec3(0, 0, 1)},
		{vec3(+0.5f, -0.5f, +0.5f), .texcoord=vec2(1, 0), /*.color=vec3(0, 1, 0)},*/ .color=vec3(1, 1, 1), .normal=vec3(0, 0, 1)},
		{vec3(+0.5f, +0.5f, +0.5f), .texcoord=vec2(1, 1), /*.color=vec3(0, 0, 1)},*/ .color=vec3(1, 1, 1), .normal=vec3(0, 0, 1)},
		{vec3(-0.5f, +0.5f, +0.5f), .texcoord=vec2(0, 1), /*.color=vec3(1, 0, 1)},*/ .color=vec3(1, 1, 1), .normal=vec3(0, 0, 1)},

		{vec3(+0.5f, -0.5f, +0.5f), .texcoord=vec2(0, 0), /*.color=vec3(1, 0, 0)},*/ .color=vec3(1, 1, 1), .normal=vec3(1, 0, 0)},
		{vec3(+0.5f, -0.5f, -0.5f), .texcoord=vec2(1, 0), /*.color=vec3(0, 1, 0)},*/ .color=vec3(1, 1, 1), .normal=vec3(1, 0, 0)},
		{vec3(+0.5f, +0.5f, -0.5f), .texcoord=vec2(1, 1), /*.color=vec3(0, 0, 1)},*/ .color=vec3(1, 1, 1), .normal=vec3(1, 0, 0)},
		{vec3(+0.5f, +0.5f, +0.5f), .texcoord=vec2(0, 1), /*.color=vec3(1, 0, 1)},*/ .color=vec3(1, 1, 1), .normal=vec3(1, 0, 0)},

		{vec3(-0.5f, -0.5f, -0.5f), .texcoord=vec2(0, 0), /*.color=vec3(1, 0, 0)},*/ .color=vec3(1, 1, 1), .normal=vec3(-1, 0, 0)},
		{vec3(-0.5f, -0.5f, +0.5f), .texcoord=vec2(1, 0), /*.color=vec3(0, 1, 0)},*/ .color=vec3(1, 1, 1), .normal=vec3(-1, 0, 0)},
		{vec3(-0.5f, +0.5f, +0.5f), .texcoord=vec2(1, 1), /*.color=vec3(0, 0, 1)},*/ .color=vec3(1, 1, 1), .normal=vec3(-1, 0, 0)},
		{vec3(-0.5f, +0.5f, -0.5f), .texcoord=vec2(0, 1), /*.color=vec3(1, 0, 1)},*/ .color=vec3(1, 1, 1), .normal=vec3(-1, 0, 0)},
	};

	// R_Projection(R_PERSPECTIVE);
	R_PerspectiveProjection(70, 320.0f/240.0f, 0.1f, 100.0f);

	R_SetTranslation(vec3(0.0f, 0.0, -1.5f - (sinf(x) * 0.5f)));
	R_SetRotation(vec3(0, x, 0));
	// R_SetRotation(vec3(0, -0.5, 0));
	R_SetScale(vec3f(0.5f));
	R_DrawQuads(verts2, 12, 0xFFAA88);

	// {
	// 	R_SetTranslation(vec3(0.0f, 0.0, -0.7f));
	// 	R_SetRotation(vec3(0, 0.0f + sinf(x)*0.2f, 0));
	// 	R_DrawQuads(verts2, 4, 0xFF8888);
	// }

	// R_DrawLineTriangles(state->cryoMesh.vertices, state->cryoMesh.vertexCount, 0xFF8888);
	// R_DrawModel(state->testboxModel, 0xFF8888);
	// R_DrawModel(state->cryoModel, 0xFF8888);
}
