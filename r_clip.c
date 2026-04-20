//
//  Created by Matt Hartley on 19/04/2026.
//  Copyright 2026 GiantJelly. All rights reserved.
//

#include <core/core.h>

#include "r_render.h"


typedef vec3_t plane_t;
plane_t PLANE_X0 = {+1, 0, 0};
plane_t PLANE_X1 = {-1, 0, 0};
plane_t PLANE_Y0 = {0, +1, 0};
plane_t PLANE_Y1 = {0, -1, 0};
plane_t PLANE_Z0 = {0, 0, +1};
plane_t PLANE_Z1 = {0, 0, -1};

typedef struct {
	vertex_t vertices[16];
	int count;
} polygon_t;

vertex_t LerpVertex(vertex_t a, vertex_t b, float t)
{
	vertex_t v;
	v.pos.x = a.pos.x + t*(b.pos.x-a.pos.x);
	v.pos.y = a.pos.y + t*(b.pos.y-a.pos.y);
	v.pos.z = a.pos.z + t*(b.pos.z-a.pos.z);
	// v.pos.w = a.pos.w + t*(b.pos.w-a.pos.w);

	v.normal.x = a.normal.x + t*(b.normal.x-a.normal.x);
	v.normal.y = a.normal.y + t*(b.normal.y-a.normal.y);
	v.normal.z = a.normal.z + t*(b.normal.z-a.normal.z);

	v.color.r = a.color.r + t*(b.color.r-a.color.r);
	v.color.g = a.color.g + t*(b.color.g-a.color.g);
	v.color.b = a.color.b + t*(b.color.b-a.color.b);
	// v.color.a = a.color.a + t*(b.color.a-a.color.a);

	v.texcoord.u = a.texcoord.u + t*(b.texcoord.u-a.texcoord.u);
	v.texcoord.v = a.texcoord.v + t*(b.texcoord.v-a.texcoord.v);

	return v;
}

float _ClipPlaneDist(vec4_t pos, plane_t plane)
{
	return pos.x*plane.x + pos.y*plane.y + pos.z*plane.z + pos.w;
}

polygon_t R_ClipWithPlane(polygon_t polygon, plane_t plane)
{
	// int previ = clip->count-1;
	// for (int vi=0; vi<clip->count; ++vi) {
	// 	rdp_vertex_t v0 = clip->verts[vi];
	// 	rdp_vertex_t v1 = clip->verts[previ];
		
	// 	float d0 = GFX_ClipPlaneDist(v0.pos, plane);
	// 	float d1 = GFX_ClipPlaneDist(v1.pos, plane);
		
	// 	bool inclip0 = d0 >= 0;
	// 	bool inclip1 = d1 >= 0;
		
	// 	if (inclip0 != inclip1) {
	// 		float t = divsafe(d0, (d0 - d1));
	// 		vertex_t v = LerpVertex(v0, v1, t);
	// 		clip->tmpVerts[clip->tmpCount++] = v;
	// 	}
		
	// 	if (inclip0) {
	// 		clip->tmpVerts[clip->tmpCount++] = clip->verts[vi];
	// 	}

	// 	previ = vi;
	// }

	// for (int idx=0; idx<clip->tmpCount; ++idx) {
	// 	clip->verts[idx] = clip->tmpVerts[idx];
	// }
	// clip->count = clip->tmpCount;
	// clip->tmpCount = 0;

	polygon_t clipped = {0};

	for (int vi=0; vi<polygon.count; ++vi) {
		vertex_t v0 = polygon.vertices[vi];
		vertex_t v1 = polygon.vertices[(vi+1)%polygon.count];

		float d0 = _ClipPlaneDist(vec4f3(v0.pos, 1), plane);
		float d1 = _ClipPlaneDist(vec4f3(v1.pos, 1), plane);

		bool inclip0 = d0 >= 0;
		bool inclip1 = d1 >= 0;

		if (inclip0) {
			clipped.vertices[clipped.count++] = v0;
		}
				
		if (inclip0 != inclip1) {
			float t = d0 / (d0 - d1);
			vertex_t v = LerpVertex(v0, v1, t);
			clipped.vertices[clipped.count++] = v;
		}
	}

	return clipped;
}

