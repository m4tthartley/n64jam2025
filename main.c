/*
	Created by Matt Hartley on 10/12/2025.
	Copyright 2025 GiantJelly. All rights reserved.
*/

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>


void DrawTestQuad(T3DVertPacked* verts, T3DMat4FP* mModelFP)
{
	// rspq_block_begin();
	t3d_matrix_push(mModelFP);
	t3d_vert_load(verts, 0, 4);
	t3d_matrix_pop(1);
	t3d_tri_draw(0, 1, 2);
	t3d_tri_draw(2, 3, 0);
	t3d_tri_sync();
	// rspq_block_t* drawBlock = rspq_block_end();
	// rspq_block_run(drawBlock);
}

typedef enum {
	TILE_NONE = 0,
	TILE_DEFAULT = 1,
} tiletype_t;
typedef struct {
	tiletype_t type;
} tile_t;

#define MAP_SIZE_X 4
#define MAP_SIZE_Y 4
tile_t map[MAP_SIZE_Y][MAP_SIZE_X] = {
	{ {1}, {1}, {1}, {1}, },
	{ {1}, {1}, {1}, {1}, },
	{ {1}, {1}, {1}, {1}, },
	{ {1}, {1}, {1}, {1}, },
};

typedef enum {
	MODEL_HALL_BASE,
	MODEL_HALL_WALL,
	MODEL_SIDE_TABLE,
	MODEL_COUNT,
} modeltype_t;
T3DModel* models[MODEL_COUNT];

#define FIXED_MAT_MAX 128
T3DMat4FP* fixedMats[FIXED_MAT_MAX];
int fixedMatIndex = 0;

int main()
{
	display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);
	rdpq_init();

	asset_init_compression(2);
	dfs_init(DFS_DEFAULT_LOCATION);

	T3DInitParams params = {};
	t3d_init(params);

	T3DMat4FP* mModelFP = malloc_uncached(sizeof(T3DMat4FP));

	for (int i=0; i<FIXED_MAT_MAX; ++i) {
		fixedMats[i] = malloc_uncached(sizeof(T3DMat4FP));
	}

	T3DVec3 cameraPos = {{0, 0.5f, -1}};
	T3DVec3 cameraTarget = {{0, 0, 0}};

	T3DVertPacked* verts = malloc_uncached(sizeof(T3DVertPacked) * 2);
	verts[0] = (T3DVertPacked){
		.posA = {-32.0f, -32.0f, 0}, .rgbaA = 0xFF0000FF, .normA = t3d_vert_pack_normal(&(T3DVec3){{0, 0, 1}}),
		.posB = {+32.0f, -32.0f, 0}, .rgbaB = 0x00FF00FF, .normB = t3d_vert_pack_normal(&(T3DVec3){{0, 0, 1}}),
	};
	verts[1] = (T3DVertPacked){
		.posA = {+32.0f, +32.0f, 0}, .rgbaA = 0x0000FFFF, .normA = t3d_vert_pack_normal(&(T3DVec3){{0, 0, 1}}),
		.posB = {-32.0f, +32.0f, 0}, .rgbaB = 0xFF0000FF, .normB = t3d_vert_pack_normal(&(T3DVec3){{0, 0, 1}}),
	};

	models[MODEL_SIDE_TABLE] = t3d_model_load("rom:/sidetable.t3dm");

	models[MODEL_HALL_BASE] = t3d_model_load("rom:/hall_floor.t3dm");

	T3DViewport viewport = t3d_viewport_create();

	float rot = 0.0f;
	for (;;) {
		fixedMatIndex = 0;
		rot += 0.01f;

		t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(90), 0.1f, 100);
		t3d_viewport_look_at(&viewport, &cameraPos, &cameraTarget, &(T3DVec3){{0,1,0}});

		T3DMat4 mModel;
		t3d_mat4_identity(&mModel);
		T3DVec3 axis = {{0.0f, 1.0f, 0.0f}};
		t3d_vec3_norm(&axis);
		t3d_mat4_rotate(&mModel, &axis, rot);
		// t3d_mat4_translate(&mModel, 0, fm_sinf(rot)*1.0f - 5, 0);
		t3d_mat4_scale(&mModel, 1.0f/64, 1.0f/64, 1.0f/64);
		t3d_mat4_to_fixed(mModelFP, &mModel);
		rdpq_attach(display_get(), display_get_zbuf());
		t3d_frame_start();
		t3d_viewport_attach(&viewport);

		rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
		t3d_screen_clear_color(RGBA32(0, 0, 20, 0));
		t3d_screen_clear_depth();

		t3d_light_set_ambient((uint8_t[]){50, 50, 50, 0xFF});
		T3DVec3 lightDir = {{1.0f, 1.0f, -1.0f}};
		t3d_vec3_norm(&lightDir);
		t3d_light_set_directional(0, (uint8_t[]){255, 255, 255, 255}, &lightDir);
		t3d_light_set_count(1);

		t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH);

		// DrawTestQuad(verts, mModelFP);

		for (int mapy=0; mapy<MAP_SIZE_Y; ++mapy)
		for (int mapx=0; mapx<MAP_SIZE_X; ++mapx) {
			assert(fixedMatIndex < FIXED_MAT_MAX);
			// T3DMat4 mModel;
			t3d_mat4_identity(&mModel);
			T3DVec3 axis2 = {{0.0f, 1.0f, 0.0f}};
			t3d_vec3_norm(&axis2);
			
			
			t3d_mat4_translate(&mModel, mapx, 0, mapy);
			t3d_mat4_rotate(&mModel, &axis, rot); 
			t3d_mat4_scale(&mModel, 1.0f/64, 1.0f/64, 1.0f/64);
			t3d_mat4_to_fixed(fixedMats[fixedMatIndex], &mModel);
			t3d_matrix_push(fixedMats[fixedMatIndex]);
			t3d_model_draw(models[MODEL_HALL_BASE]);
			t3d_matrix_pop(1);

			++fixedMatIndex;
		}

		rdpq_detach_show();
	}
}