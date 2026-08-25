
#include <core/core.h>
#include "r_render.h"


extern uint32_t* framebuffer;
extern float* depthbuffer;
extern vec3_t __rTranslation;
extern vec3_t __rRotation;
extern vec3_t __rScale;
extern texture_t* __rActiveTexture;
extern projection_type_t __rProjectionType;
extern perspective_factors_t __rPerspectiveFactors;


void R_RasterizeBBoxTriangle(vertex_t v0, vertex_t v1, vertex_t v2, color_t color)
{
	vidrect_t res = {0, 0, 320, 240};

	// R_RasterizeLine(v0.pos, v1.pos, color);
	// R_RasterizeLine(v1.pos, v2.pos, color);
	// R_RasterizeLine(v0.pos, v2.pos, color);

	// Divide by W
	v0.pos.xyz = div3(v0.pos.xyz, vec3f(v0.pos.w));
	v1.pos.xyz = div3(v1.pos.xyz, vec3f(v1.pos.w));
	v2.pos.xyz = div3(v2.pos.xyz, vec3f(v2.pos.w));

	// Calc reciprocals of UV, Color and W for perspective correction
	v0.texcoord = div2(v0.texcoord, vec2f(v0.pos.w));
	v1.texcoord = div2(v1.texcoord, vec2f(v1.pos.w));
	v2.texcoord = div2(v2.texcoord, vec2f(v2.pos.w));
	v0.color = div3(v0.color, vec3f(v0.pos.w));
	v1.color = div3(v1.color, vec3f(v1.pos.w));
	v2.color = div3(v2.color, vec3f(v2.pos.w));
	v0.pos.w = 1.0f / v0.pos.w;
	v1.pos.w = 1.0f / v1.pos.w;
	v2.pos.w = 1.0f / v2.pos.w;

	// Transform NDC to framebuffer coords
	v0.pos = ClipSpaceToFramebufferSpace(v0.pos);
	v1.pos = ClipSpaceToFramebufferSpace(v1.pos);
	v2.pos = ClipSpaceToFramebufferSpace(v2.pos);

	float area = TriangleArea(v0.pos.xy, v1.pos.xy, v2.pos.xy);
	if (area < 0.0f) {
		SWAP(v1, v2);
	}

	float left = min(v0.pos.x, min(v1.pos.x, v2.pos.x));
	float right = max(v0.pos.x, max(v1.pos.x, v2.pos.x));
	float bottom = min(v0.pos.y, min(v1.pos.y, v2.pos.y));
	float top = max(v0.pos.y, max(v1.pos.y, v2.pos.y));
	int ileft = max(left, 0);
	int iright = min(right, res.w);
	int ibottom = max(bottom, 0);
	int itop = min(top, res.h);

	for (int y=ibottom; y<=itop; ++y) {
		for (int x=ileft; x<=iright; ++x) {
			vec3_t t = BarycentricCoords(vec2(x, y), v0.pos.xy, v1.pos.xy, v2.pos.xy);
			if (t.x < 0 || t.y < 0 || t.z < 0) {
				continue;
			}

			if (x==iright && y==120) {
				int asd = 0;
			}

			vertex_t v = LerpTriVetices(vec2(x, y), v0, v1, v2);
			float w = 1.0f / v.pos.w;

			if (depthbuffer[y*res.w + x] < w) {
				continue;
			}

			v.texcoord = mul2(v.texcoord, vec2f(w));
			v.color = mul3(v.color, vec3f(w));

			// color_t combinedColor = MixColor32(, color);
			// color = Color32(v.color.r, v.color.g, v.color.b, 0);
			color_t vertexColor = Color32(v.color.r, v.color.g, v.color.b, 0);

			uint32_t texel = 0xFFFFFFFF;

			if (x==iright) {
				int asd = 0;
			}

			if (__rActiveTexture) {
				int tx = (float)__rActiveTexture->width * v.texcoord.x;
				int ty = (float)__rActiveTexture->height * v.texcoord.y;
				if (ty >= 0 && tx >= 0) {
					texel = __rActiveTexture->texels[ty*__rActiveTexture->width+tx];
					// texel = Color32(v.texcoord.x, v.texcoord.y, 0, 0);
				}

				uint32_t texCoordViz =
					((uint32_t)((float)tx*8)<<0) |
					((uint32_t)((float)ty*8)<<8);
			}

			framebuffer[y*res.w + x] = MixColor32(MixColor32(color, vertexColor), texel);
			depthbuffer[y*res.w + x] = w;
		}
	}
}
