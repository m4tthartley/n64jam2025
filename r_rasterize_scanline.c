
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


// void _RasterizeTriangleHalf(
// 	vertex_t v0,
// 	vertex_t v1,
// 	vertex_t v2,
// 	float lineStart,
// 	float lineEnd,
// 	float startStep,
// 	float endStep,
// 	vec2_t startPoint,
// 	vec2_t endPoint
// )
// {
// 	vidrect_t res = {0, 0, 320, 240};

// 	for (int l=v0.pos.y; l<=v1.pos.y; ++l) {
// 		float start = lineStart;
// 		float end = lineEnd;
// 		// if (startPoint.x > v0.x) {
// 		// 	start = min(lineStart, startPoint.x);
// 		// } else {
// 		// 	start = max(lineStart, startPoint.x);
// 		// }
// 		// if (endPoint.x > v0.x) {
// 		// 	end = min(lineEnd, endPoint.x);
// 		// } else {
// 		// 	end = max(lineEnd, endPoint.x);
// 		// }
// 		for (int x=start; x<=end; ++x) {
// 			if ((int)v1.pos.y-1 == l) {
// 				int asd = 0;
// 			}

// 			if (startPoint.x < v0.pos.x && x < startPoint.x) {
// 				continue;
// 			}
// 			if (endPoint.x > v0.pos.x && x > endPoint.x) {
// 				continue;
// 			}

// 			// vec3_t triCoords = BarycentricCoords(vec2(x, l), v0.pos.xy, v1.pos.xy, v2.pos.xy);
// 			// vec3_t color3 = {
// 			// 	v0.color.r*triCoords.f[0] + v1.color.r*triCoords.f[1] + v2.color.r*triCoords.f[2],
// 			// 	v0.color.g*triCoords.f[0] + v1.color.g*triCoords.f[1] + v2.color.g*triCoords.f[2],
// 			// 	v0.color.b*triCoords.f[0] + v1.color.b*triCoords.f[1] + v2.color.b*triCoords.f[2],
// 			// };

// 			vec3_t t = BarycentricCoords(vec2(x, l), v0.pos.xy, v1.pos.xy, v2.pos.xy);

// 			// if (t.x <= 0 || t.y <= 0 || t.z <= 0) {
// 			// 	continue;
// 			// }

// 			vertex_t v = LerpTriVetices(vec2(x, l), v0, v1, v2);
// 			float w = 1.0f / v.pos.w;

// 			if (depthbuffer[l*res.w + x] < w) {
// 				continue;
// 			}

// 			v.texcoord = mul2(v.texcoord, vec2f(w));
// 			v.color = mul3(v.color, vec3f(w));

// 			uint32_t triColorViz =
// 				((uint32_t)(t.r*255.0f)<<0) |
// 				((uint32_t)(t.g*255.0f)<<8) |
// 				((uint32_t)(t.b*255.0f)<<16);

// 			uint32_t color =
// 				((uint32_t)(v.color.r*255.0f)<<0) |
// 				((uint32_t)(v.color.g*255.0f)<<8) |
// 				((uint32_t)(v.color.b*255.0f)<<16);

// 			int tx = (float)__rActiveTexture->width * v.texcoord.x;
// 			int ty = (float)__rActiveTexture->height * v.texcoord.y;
// 			uint32_t texel = __rActiveTexture->texels[ty*__rActiveTexture->width+tx];

// 			uint32_t texCoordViz =
// 				((uint32_t)((float)tx*8)<<0) |
// 				((uint32_t)((float)ty*8)<<8);

// 			if (l < 0 || l >= res.h || x < 0 || x >= res.w) {
// 				continue; // TODO: Shouldn't be needed
// 			}

// 			framebuffer[l*res.w + x] = MixColor32(color, texel);
// 			// framebuffer[l*res.w + x] = color;
// 			// framebuffer[l*res.w + x] = (1.0f / v.pos.w) * 255.0f;
// 			depthbuffer[l*res.w + x] = w;
// 		}

// 		lineStart += startStep;
// 		lineEnd += endStep;
// 	}
// }

void R_RasterizeScanlineTriangle(vertex_t v0, vertex_t v1, vertex_t v2, color_t color)
{
	vidrect_t res = {0, 0, 320, 240};

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
	// if ((int)v0.pos.y != (int)v1.pos.y) {
		// _RasterizeTriangleHalf(v0, v1, v2, lineStart, lineEnd, startStep, endStep, startPoint, endPoint);

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
				if ((int)v1.pos.y-1 == l) {
					int asd = 0;
				}

				if (startPoint.x < v0.pos.x && x < startPoint.x) {
					continue;
				}
				if (endPoint.x > v0.pos.x && x > endPoint.x) {
					continue;
				}

				// vec3_t triCoords = BarycentricCoords(vec2(x, l), v0.pos.xy, v1.pos.xy, v2.pos.xy);
				// vec3_t color3 = {
				// 	v0.color.r*triCoords.f[0] + v1.color.r*triCoords.f[1] + v2.color.r*triCoords.f[2],
				// 	v0.color.g*triCoords.f[0] + v1.color.g*triCoords.f[1] + v2.color.g*triCoords.f[2],
				// 	v0.color.b*triCoords.f[0] + v1.color.b*triCoords.f[1] + v2.color.b*triCoords.f[2],
				// };

				vec3_t t = BarycentricCoords(vec2(x, l), v0.pos.xy, v1.pos.xy, v2.pos.xy);

				// if (t.x <= 0 || t.y <= 0 || t.z <= 0) {
				// 	continue;
				// }

				vertex_t v = LerpTriVetices(vec2(x, l), v0, v1, v2);
				float w = 1.0f / v.pos.w;

				if (depthbuffer[l*res.w + x] < w) {
					continue;
				}

				v.texcoord = mul2(v.texcoord, vec2f(w));
				v.color = mul3(v.color, vec3f(w));

				uint32_t triColorViz =
					((uint32_t)(t.r*255.0f)<<0) |
					((uint32_t)(t.g*255.0f)<<8) |
					((uint32_t)(t.b*255.0f)<<16);

				uint32_t color =
					((uint32_t)(v.color.r*255.0f)<<0) |
					((uint32_t)(v.color.g*255.0f)<<8) |
					((uint32_t)(v.color.b*255.0f)<<16);

				uint32_t texel = 0xFFFFFFFF;

				if (__rActiveTexture) {
					int tx = (float)__rActiveTexture->width * v.texcoord.x;
					int ty = (float)__rActiveTexture->height * v.texcoord.y;
					texel = __rActiveTexture->texels[ty*__rActiveTexture->width+tx];

					uint32_t texCoordViz =
						((uint32_t)((float)tx*8)<<0) |
						((uint32_t)((float)ty*8)<<8);
				}
				// uint32_t texel = 0xFFFFFFFF;

				if (l < 0 || l >= res.h || x < 0 || x >= res.w) {
					continue; // TODO: Shouldn't be needed
				}

				framebuffer[l*res.w + x] = MixColor32(color, texel);
				// framebuffer[l*res.w + x] = color;
				// framebuffer[l*res.w + x] = (1.0f / v.pos.w) * 255.0f;
				depthbuffer[l*res.w + x] = w;
			}

			lineStart += startStep;
			lineEnd += endStep;
		}
	// }

	// Second half
	lineStart = longEdgePoint.x;
	lineEnd = v1.pos.x;
	if (isnan(lineStart) || isnan(lineEnd)) {
		return;
	}
	startStep = (v2.pos.x-longEdgePoint.x) / (v2.pos.y-longEdgePoint.y);
	endStep = (v2.pos.x-v1.pos.x) / (v2.pos.y-v1.pos.y);
	if (v1.pos.x < longEdgePoint.x) {
		SWAP(lineStart, lineEnd);
		SWAP(startStep, endStep);
	}

	// _RasterizeTriangleHalf(v0, v1, v2, lineStart, lineEnd, startStep, endStep, startPoint, endPoint);

	// if ((int)v2.y != (int)v1.y) {
		for (int l=v1.pos.y; l<=v2.pos.y; ++l) {
			for (int x=lineStart; x<=lineEnd; ++x) {
				vec3_t t = BarycentricCoords(vec2(x, l), v0.pos.xy, v1.pos.xy, v2.pos.xy);
				uint32_t triColorViz =
					((uint32_t)(t.r*255.0f)<<0) |
					((uint32_t)(t.g*255.0f)<<8) |
					((uint32_t)(t.b*255.0f)<<16);

				vertex_t v = LerpTriVetices(vec2(x, l), v0, v1, v2);
				float w = 1.0f / v.pos.w;
				if (isnan(w)) {
					int x = 0;
				}

				if (depthbuffer[l*res.w + x] < w) {
					continue;
				}

				v.texcoord = mul2(v.texcoord, vec2f(1.0f / v.pos.w));
				v.color = mul3(v.color, vec3f(1.0f / v.pos.w));

				uint32_t color =
					((uint32_t)(v.color.r*255.0f)<<0) |
					((uint32_t)(v.color.g*255.0f)<<8) |
					((uint32_t)(v.color.b*255.0f)<<16);

				uint32_t texel = 0xFFFFFFFF;

				if (__rActiveTexture) {
					int tx = (float)__rActiveTexture->width * v.texcoord.x;
					int ty = (float)__rActiveTexture->height * v.texcoord.y;
					texel = __rActiveTexture->texels[ty*__rActiveTexture->width+tx];

					uint32_t texCoordViz =
						((uint32_t)((float)tx*8)<<0) |
						((uint32_t)((float)ty*8)<<8);
				}
				// uint32_t texel = 0xFFFFFFFF;

				if (l < 0 || l >= res.h || x < 0 || x >= res.w) {
					continue; // TODO: Shouldn't be needed
				}

				framebuffer[l*res.w + x] = MixColor32(color, texel);
				// framebuffer[l*res.w + x] = color;
				// framebuffer[l*res.w + x] = (1.0f / v.pos.w) * 255.0f;
				depthbuffer[l*res.w + x] = w;
			}

			lineStart += startStep;
			lineEnd += endStep;
		}
	// }

	// Debug lines
	// R_RasterizeLine(v0.pos, v1.pos, color);
	// R_RasterizeLine(v1.pos, v2.pos, color);
	// R_RasterizeLine(v0.pos, v2.pos, color);

	// R_DrawLine(v0.pos.x, v0.pos.y, v1.pos.x, v1.pos.y, color);
	// R_DrawLine(v1.pos.x, v1.pos.y, v2.pos.x, v2.pos.y, color);
	// R_DrawLine(v0.pos.x, v0.pos.y, v2.pos.x, v2.pos.y, color);
}
