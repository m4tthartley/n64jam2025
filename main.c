/*
	Created by Matt Hartley on 10/12/2025.
	Copyright 2025 GiantJelly. All rights reserved.
*/

#include "debugcpp.h"
#include "joypad.h"
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
	MODEL_CRYOPOD_BASE,
	MODEL_COUNT,
} modeltype_t;
T3DModel* models[MODEL_COUNT];

#define FIXED_MAT_MAX 128
T3DMat4FP* fixedMats[FIXED_MAT_MAX];
int fixedMatIndex = 0;

void Mat4Rotate(T3DMat4 *matOut, const T3DVec3* axis, float angleRad)
{
	float s, c;
	// @TODO: currently buggy in libdragon, use once fixed
	// fm_sincosf(angleRad, &s, &c);
	s = fm_sinf(angleRad);
	c = fm_cosf(angleRad);

	float t = 1.0f - c;

	float x = axis->v[0];
	float y = axis->v[1];
	float z = axis->v[2];

	T3DMat4 mat;

	mat.m[0][0] = t * x * x + c;
	mat.m[0][1] = t * x * y - s * z;
	mat.m[0][2] = t * x * z + s * y;
	mat.m[0][3] = 0.0f;

	mat.m[1][0] = t * x * y + s * z;
	mat.m[1][1] = t * y * y + c;
	mat.m[1][2] = t * y * z - s * x;
	mat.m[1][3] = 0.0f;

	mat.m[2][0] = t * x * z - s * y;
	mat.m[2][1] = t * y * z + s * x;
	mat.m[2][2] = t * z * z + c;
	mat.m[2][3] = 0.0f;

	mat.m[3][0] = 0.0f;
	mat.m[3][1] = 0.0f;
	mat.m[3][2] = 0.0f;
	mat.m[3][3] = 1.0f;

	T3DMat4 matA = *matOut;
	t3d_mat4_mul(matOut, &mat, &matA);
}

int main()
{
	display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);
	rdpq_init();

	asset_init_compression(2);
	dfs_init(DFS_DEFAULT_LOCATION);

	joypad_init();

	T3DInitParams params = {};
	t3d_init(params);

	T3DMat4FP* mModelFP = malloc_uncached(sizeof(T3DMat4FP));

	for (int i=0; i<FIXED_MAT_MAX; ++i) {
		fixedMats[i] = malloc_uncached(sizeof(T3DMat4FP));
	}

	T3DVec3 cameraPos = {{0, 0.5f, -1}};

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
	models[MODEL_HALL_WALL] = t3d_model_load("rom:/hall_wall.t3dm");

	models[MODEL_CRYOPOD_BASE] = t3d_model_load("rom:/cryopod_base.t3dm");

	T3DViewport viewport = t3d_viewport_create();

	// surface_t* display = display_get();
	// graphics_draw_text(display, 10, 10, "Game");

	// console_init();

	float rot = 0.0f;
	float rotX = 0.0f;
	float rotY = 0.0f;
	for (;;) {
		// console_clear();
		printf("Game \n");
		joypad_poll();

		JOYPAD_PORT_FOREACH(port) {
			joypad_accessory_type_t type = joypad_get_accessory_type(port);
			joypad_style_t style = joypad_get_style(port);
			joypad_inputs_t inputs = joypad_get_inputs(port);

			printf("controller %i \n", type);
			if (style == JOYPAD_STYLE_N64) {
				rotY += (float)inputs.stick_x * 0.001f;
				rotX += (float)inputs.stick_y * 0.001f;

				if (inputs.btn.d_up) {
					cameraPos.z += 0.01f;
				}
				if (inputs.btn.d_down) {
					cameraPos.z += -0.01f;
				}
				if (inputs.btn.d_left) {
					cameraPos.x += 0.01f;
				}
				if (inputs.btn.d_right) {
					cameraPos.x -= 0.01f;
				}
			}
			// if (type == JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) {
			// }
		}

		// console_render();

		fixedMatIndex = 0;
		// rot += 0.02f;
		T3DVec3 axisY = {{0.0f, 1.0f, 0.0f}};
		T3DVec3 axisX = {{1.0f, 0.0f, 0.0f}};
		t3d_vec3_norm(&axisY);
		t3d_vec3_norm(&axisX);

		t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(90), 0.1f, 100);

		T3DVec3 cameraTarget;
		T3DVec3 dir = {{0, 0, 1}};
		T3DVec4 rotatedDir = {{0, 0, 1}};
		T3DMat4 viewMatrix;
		t3d_mat4_identity(&viewMatrix);
		Mat4Rotate(&viewMatrix, &axisX, rotX);
		Mat4Rotate(&viewMatrix, &axisY, rotY);
		// t3d_mat4_from_srt_euler(&viewMatrix, (float[]){1, 1, 1}, (float[]){rotX, rotY, 0}, (float[]){0, 0, 0});
		t3d_mat4_mul_vec3(&rotatedDir, &viewMatrix, &dir);
		dir.x = rotatedDir.x;
		dir.y = rotatedDir.y;
		dir.z = rotatedDir.z;
		t3d_vec3_add(&cameraTarget, &cameraPos, &dir);
		t3d_viewport_look_at(&viewport, &cameraPos, &cameraTarget, &(T3DVec3){{0,1,0}});
		// T3DMat4 viewMatrix;
		// t3d_mat4_identity(&viewMatrix);
		// t3d_mat4_rotate(&viewMatrix, &axis, rot);
		// t3d_mat4_scale(&viewMatrix, 1.0f, 1.0f, -1.0f);
		// t3d_viewport_set_view_matrix(&viewport, &viewMatrix);

		T3DMat4 mModel;
		t3d_mat4_identity(&mModel);
		t3d_mat4_rotate(&mModel, &axisY, rot);
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

		t3d_mat4_identity(&mModel);
		t3d_mat4_scale(&mModel, 1.0f/64, 1.0f/64, 1.0f/64);
		t3d_mat4_translate(&mModel, 0, -1, 0);
		t3d_mat4_to_fixed(fixedMats[fixedMatIndex], &mModel);
		t3d_matrix_push(fixedMats[fixedMatIndex]);
		t3d_model_draw(models[MODEL_CRYOPOD_BASE]);
		t3d_matrix_pop(1);
		++fixedMatIndex;

		for (int mapy=0; mapy<MAP_SIZE_Y; ++mapy) {
			for (int mapx=0; mapx<MAP_SIZE_X; ++mapx) {
				assert(fixedMatIndex < FIXED_MAT_MAX);
				// T3DMat4 mModel;
				mModel = (T3DMat4){0};
				t3d_mat4_identity(&mModel);

				T3DVec3 axis2 = {{0.0f, 1.0f, 0.0f}};
				t3d_vec3_norm(&axis2);
				
				// Mat4Rotate(&mModel, &axis2, rot);
				t3d_mat4_scale(&mModel, 1.0f/64, 1.0f/64, 1.0f/64);
				t3d_mat4_translate(&mModel, mapx, 0, mapy);

				t3d_mat4_to_fixed(fixedMats[fixedMatIndex], &mModel);
				t3d_matrix_push(fixedMats[fixedMatIndex]);

				t3d_model_draw(models[MODEL_HALL_WALL]);
				
				if (mapx == (MAP_SIZE_X-1)) {
					// t3d_model_draw(models[MODEL_HALL_WALL]);
				}

				t3d_matrix_pop(1);

				++fixedMatIndex;
			}
		}

		rdpq_detach_show();
	}
}