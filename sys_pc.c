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
#include <core/gltf.h>
#include <core/bmp.h>

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

texture_t R_LoadTexture(char* path)
{
	state_t* state = GetState();

	file_data_t* file = Sys_LoadFile(&state->assetArena, path);

	texture_t result;
	bmp_info_t info = bmp_get_info(file->data);
	result.texels = alloc_memory(&state->assetArena, info.width*info.height*sizeof(color_t));
	result.width = info.width;
	result.height = info.height;
	bmp_load_rgba32(file->data, result.texels);

	for (int idx=0; idx<result.width*result.height; ++idx) {
		vec4_t c = Color32ToFloat(result.texels[idx]);
		result.texels[idx] = Color32(c.b, c.g, c.r, c.a);
	}

	free_memory(&state->assetArena, file);

	return result;
}

mesh_t ConvertToMesh(gltf_model_t* gltf)
{
	// NOTE: This is currently just translating the model containing multiple meshes
	// 		into a single mesh for simplicity

	int vertexCount = 0;
	for (int idx=0; idx<gltf->meshCount; ++idx) {
		vertexCount += gltf->meshes[idx].vertexCount;
	}

	int size = vertexCount * sizeof(mvertex_t);

	state_t* state = GetState();

	mesh_t mesh = {
		.vertices = alloc_memory(&state->assetArena, size),
		.vertexCount = vertexCount,
	};

	for (int mi=0; mi<gltf->meshCount; ++mi) {
		for (int idx=0; idx<gltf->meshes[mi].vertexCount; ++idx) {
			mesh.vertices[idx] = (mvertex_t){
				.pos = gltf->meshes[mi].vertices[idx].pos,
				.normal = gltf->meshes[mi].vertices[idx].normal,
				.color = gltf->meshes[mi].vertices[idx].color,
				.texcoord = gltf->meshes[mi].vertices[idx].uv,
			};
		}
	}

	return mesh;
}

model_t ConvertToModel(gltf_model_t* gltf)
{
	state_t* state = GetState();

	mesh_t* meshes = alloc_memory(&state->assetArena, sizeof(mesh_t)*gltf->meshCount);

	for (int i=0; i<gltf->meshCount; ++i) {
		gltf_mesh_t* gmesh = gltf->meshes + i;
		meshes[i].vertexCount = gltf->meshes[i].vertexCount;
		meshes[i].vertices = alloc_memory(&state->assetArena, sizeof(mvertex_t)*meshes[i].vertexCount);
		for (int vi=0; vi<meshes[i].vertexCount; ++vi) {
			meshes[i].vertices[vi] = (mvertex_t){
				.pos = gltf->meshes[i].vertices[vi].pos,
				.normal = gltf->meshes[i].vertices[vi].normal,
				.color = gltf->meshes[i].vertices[vi].color,
				.texcoord = gltf->meshes[i].vertices[vi].uv,
			};
		}

		if (strsize(gmesh->textureFile)) {
			meshes[i].texture = R_LoadTexture(gmesh->textureFile);
		}
	}

	model_t model = {
		.meshCount = gltf->meshCount,
		.meshes = meshes,
	};

	return model;
}

model_t LoadGltfModel(char* path)
{
	file_data_t* file = Sys_LoadFile(&__state->assetArena, path);
	gltf_model_t gltf = gltf_load(file->data);
	model_t model = ConvertToModel(&gltf);
	return model;
}

void Init(state_t* state)
{
	state->assetArena = virtual_heap_allocator(MB(100), MB(1));

	file_data_t* gltf = Sys_LoadFile(&__state->assetArena, "assets/cryopod_base.glb");
	gltf_model_t cryoGltf = gltf_load(gltf->data);
	state->cryoMesh = ConvertToMesh(&cryoGltf);
	state->cryoModel = ConvertToModel(&cryoGltf);
	state->cubeModel = LoadGltfModel("assets/cube.glb");
	state->testboxModel = LoadGltfModel("assets/testbox.glb");

	state->window = vid_init_window("Linux Window", 320*4, 240*4, 0);

	vid_init_opengl(&state->window);

	state->framebuffer = sys_alloc_memory(320*240*sizeof(color_t));
	state->depthbuffer = sys_alloc_memory(320*240*sizeof(float));

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
	depthbuffer = state->depthbuffer;
	window_t* vid = &state->window;
	vid_poll_events(vid);

	if (vid->quit) {
		exit(0);
	}

	
	Render3DModelTestScene();
	
	// for (int idx=0; idx<320*240; ++idx) {
	// 	framebuffer[idx] = depthbuffer[idx] * 100.0f;
	// }

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