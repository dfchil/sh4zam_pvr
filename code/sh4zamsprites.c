/** Various examples of sprites based rendering of cubes and wireframes on the
 * Dreamcast using KallistiOS. By Daniel Fairchild, aka dRxL, @dfchil,
 * daniel@fairchild.dk */

#include <dc/pvr.h> /* PVR library headers for PowerVR graphics chip functions */
#include <kos.h> /* Includes necessary KallistiOS (KOS) headers for Dreamcast development */
#include <stdio.h> /* Standard I/O library headers for input and output functions */
#include <stdlib.h> /* Standard library headers for general-purpose functions, including abs() */

// #define DEBUG
#ifdef DEBUG
#include <arch/gdb.h>
#endif

#define SUPERSAMPLING 1 // Set to 1 to enable horizontal FSAA, 0 to disable
#if SUPERSAMPLING == 1
#define XSCALE 2.0f
#else
#define XSCALE 1.0f
#endif

#ifndef SHOWFRAMETIMES
#define SHOWFRAMETIMES 0
#endif

#include <sh4zamsprites/cube.h> /* Cube vertices and side strips layout */
#include <sh4zamsprites/perspective.h> /* Perspective projection matrix functions */
#include <sh4zamsprites/tex_loader.h> /* texture management */

#include <sh4zam/shz_sh4zam.h>

#define DEFAULT_FOV 75.0f // Field of view, adjust with dpad up/down
#define ZOOM_SPEED 0.3f
#define MODEL_SCALE 3.0f
#define MIN_ZOOM -10.0f
#define MAX_ZOOM 15.0f
#define LINE_WIDTH 1.0f
#define WIREFRAME_MIN_GRID_LINES 0
#define WIREFRAME_MAX_GRID_LINES 10
#define WIREFRAME_GRID_LINES_STEP 5
typedef enum : uint8_t {
  TEXTURED_TR = 0,  // Textured transparent cube
  CUBES_CUBE_MIN,   // Cube of cubes, 7x7x7 cubes, in 6 different color
                    // variations
  CUBES_CUBE_MAX,   // Cube of cubes, 15x15x15 cubes, no color variations
  WIREFRAME_EMPTY,  // Wireframe cube, colored wires on the sides only
  WIREFRAME_FILLED, // Wireframe cube, as above, but with a white grid inside
  MAX_RENDERMODE    // Not a render mode, sentinel value for end of enum
} render_mode_e;

static render_mode_e render_mode = TEXTURED_TR;
static float fovy = DEFAULT_FOV;
static uint32_t dpad_right_down = 0;

static const alignas(32) uint8_t texture256_raw[] = {
#embed "../build/pvrtex/rgb565_vq_tw/sh4zam256.dt"
};
static const alignas(32) uint8_t texture128_raw[] = {
#embed "../build/pvrtex/argb1555_vq_tw/sh4zam128_t.dt"
};
static const alignas(32) uint8_t texture32_raw[] = {
#embed "../build/pvrtex/pal4/sh4zam32_w.dt"
    // #embed "../build/pvrtex/rgb565_vq_tw/sh4zam32.dt"
};
static const alignas(32) uint8_t palette32_raw[] = {
#embed "../build/pvrtex/pal4/sh4zam32_w.dt.pal"
};


static const alignas(32) uint8_t teapot_stl[] = {
#embed "../assets/models/Utah_teapot_(solid).stl"
};


static alignas(32) dttex_info_t texture256x256;
static alignas(32) dttex_info_t texture128x128;
static alignas(32) dttex_info_t texture32x32;

static inline void set_cube_transform(float scale) {
  alignas(32) shz_mat4x4_t wmat = {0};
  shz_xmtrx_init_translation(cube_state.pos.x, cube_state.pos.y,
                             cube_state.pos.z);
  shz_xmtrx_apply_scale(scale * MODEL_SCALE * XSCALE, scale * MODEL_SCALE, scale * MODEL_SCALE);
  shz_xmtrx_apply_rotation_x(cube_state.rot.x);
  shz_xmtrx_apply_rotation_y(cube_state.rot.y);
  shz_xmtrx_store_4x4(&wmat);

  shz_xmtrx_load_4x4(&stored_projection_view);
  shz_xmtrx_apply_4x4(&wmat);
}

static inline void draw_textured_sprite(shz_vec4_t *tverts, uint32_t side,
                                        pvr_dr_state_t *dr_state) {
  shz_vec4_t *ac = tverts + cube_side_strips[side][0];
  shz_vec4_t *bc = tverts + cube_side_strips[side][2];
  shz_vec4_t *cc = tverts + cube_side_strips[side][3];
  shz_vec4_t *dc = tverts + cube_side_strips[side][1];
  pvr_sprite_txr_t *quad = (pvr_sprite_txr_t *)pvr_dr_target(*dr_state);
  quad->flags = PVR_CMD_VERTEX_EOL;
  quad->ax = ac->x;
  quad->ay = ac->y;
  quad->az = ac->z;
  quad->bx = bc->x;
  quad->by = bc->y;
  quad->bz = bc->z;
  quad->cx = cc->x;
  pvr_dr_commit(quad);
  quad = (pvr_sprite_txr_t *)pvr_dr_target(*dr_state);
  /* make a pointer with 32 bytes negative offset to allow field access to the
   * second half of the quad */
  pvr_sprite_txr_t *quad2ndhalf = (pvr_sprite_txr_t *)((int)quad - 32);
  quad2ndhalf->cy = cc->y;
  quad2ndhalf->cz = cc->z;
  quad2ndhalf->dx = dc->x;
  quad2ndhalf->dy = dc->y;
  quad2ndhalf->auv =
      PVR_PACK_16BIT_UV(cube_tex_coords[0][0], cube_tex_coords[0][1]);
  quad2ndhalf->cuv =
      PVR_PACK_16BIT_UV(cube_tex_coords[3][0], cube_tex_coords[3][1]);
  quad2ndhalf->buv =
      PVR_PACK_16BIT_UV(cube_tex_coords[2][0], cube_tex_coords[2][1]);
  pvr_dr_commit(quad);
}

void render_txr_tr_cube(void) {
  set_cube_transform(1.0f);
  alignas(32) shz_vec4_t tverts[8] = {0};

  for (int i = 0; i < 8; i++) {
    tverts[i] = shz_xmtrx_transform_vec4(cube_vertices[i]);
    tverts[i].z = shz_invf_fsrra(tverts[i].w);
    tverts[i].x *= tverts[i].z;
    tverts[i].y *= tverts[i].z;
  }

  pvr_dr_state_t dr_state;
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_txr(&cxt, PVR_LIST_TR_POLY, texture256x256.pvrformat,
                     texture256x256.width, texture256x256.height,
                     texture256x256.ptr, PVR_FILTER_BILINEAR);
  cxt.gen.specular = PVR_SPECULAR_ENABLE;
  cxt.gen.culling = PVR_CULLING_NONE;
  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);
  hdr.argb = 0x7FFFFFFF;
  for (int i = 0; i < 6; i++) {
    pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
    *hdrpntr = hdr;
    hdrpntr->oargb = cube_side_colors[i];
    pvr_dr_commit(hdrpntr);
    draw_textured_sprite(tverts, i, &dr_state);
  }
  pvr_dr_finish();
}

void render_cubes_cube() {
  set_cube_transform(1.0f);
  pvr_sprite_cxt_t cxt;

  pvr_list_type_t list_type =
      (render_mode == CUBES_CUBE_MAX) ? PVR_LIST_OP_POLY : PVR_LIST_PT_POLY;

  pvr_sprite_cxt_col(&cxt, list_type);
  uint32_t cuberoot_cubes = 3;
  if (render_mode == CUBES_CUBE_MAX) {
    cuberoot_cubes = 17 - (SUPERSAMPLING * 1);
    // 15x15x15 cubes, 6 faces per cube, 2 triangles per face @60 fps ==
    // 2430000 triangles pr. second 17*17*16 cubes, or 3329280 triangles pr.
    // second, works with FSAA disabled, set #define SUPERSAMPLING 0
    pvr_sprite_cxt_txr(&cxt, list_type,
                       texture32x32.pvrformat | PVR_TXRFMT_4BPP_PAL(16),
                       texture32x32.width, texture32x32.height,
                       texture32x32.ptr, PVR_FILTER_BILINEAR);
    // cxt.gen.specular = PVR_SPECULAR_DISABLE;
  } else {
    pvr_sprite_cxt_txr(&cxt, list_type, texture128x128.pvrformat,
                       texture128x128.width, texture128x128.height,
                       texture128x128.ptr, PVR_FILTER_NEAREST);
  }
  cxt.gen.specular = PVR_SPECULAR_ENABLE;
  cxt.gen.culling = PVR_CULLING_NONE;
  pvr_dr_state_t dr_state;
  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);
  hdr.argb = 0xFFFFFFFF;
  if (render_mode == CUBES_CUBE_MAX) { // use single shared header for MAX
                                       // mode without specular
    pvr_sprite_hdr_t *hdrptr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
    *hdrptr = hdr;
    pvr_dr_commit(hdrptr);
  }
  shz_vec4_t *cube_min = cube_vertices + 6;
  shz_vec4_t *cube_max = cube_vertices + 3;
  shz_vec4_t cube_step = {
      .e = {shz_divf_fsrra((cube_max->x - cube_min->x), cuberoot_cubes),
            shz_divf_fsrra((cube_max->y - cube_min->y), cuberoot_cubes),
            shz_divf_fsrra((cube_max->z - cube_min->z), cuberoot_cubes), 1.0f}};
  shz_vec4_t cube_size = {.e = {cube_step.x * 0.75f, cube_step.y * 0.75f,
                                cube_step.z * 0.75f, 1.0f}};
  int xiterations =
      cuberoot_cubes -
      (SUPERSAMPLING == 0 && render_mode == CUBES_CUBE_MAX ? 1 : 0);

  for (int cx = 0; cx < xiterations; cx++) {
    for (uint32_t cy = 0; cy < cuberoot_cubes; cy++) {
      for (uint32_t cz = 0; cz < cuberoot_cubes; cz++) {
        if (render_mode == CUBES_CUBE_MIN) {
          pvr_sprite_hdr_t *hdrpntr =
              (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
          *hdrpntr = hdr;
          hdrpntr->oargb = cube_side_colors[(cx + cy + cz) % 6];
          pvr_dr_commit(hdrpntr);
        }
        shz_vec4_t cube_pos = {.e = {cube_min->x + cube_step.x * (float)cx,
                                     cube_min->y + cube_step.y * (float)cy,
                                     cube_min->z + cube_step.z * (float)cz,
                                     1.0f}};
        alignas(32) shz_vec4_t tverts[8] = {
            shz_xmtrx_transform_vec4((shz_vec4_t){.x = cube_pos.x,
                                                  .y = cube_pos.y,
                                                  .z = cube_pos.z + cube_size.z,
                                                  .w = 1.0f}),
            shz_xmtrx_transform_vec4((shz_vec4_t){.x = cube_pos.x,
                                                  .y = cube_pos.y + cube_size.y,
                                                  .z = cube_pos.z + cube_size.z,
                                                  .w = 1.0f}),
            shz_xmtrx_transform_vec4((shz_vec4_t){.x = cube_pos.x + cube_size.x,
                                                  .y = cube_pos.y,
                                                  .z = cube_pos.z + cube_size.z,
                                                  .w = 1.0f}),
            shz_xmtrx_transform_vec4((shz_vec4_t){.x = cube_pos.x + cube_size.x,
                                                  .y = cube_pos.y + cube_size.y,
                                                  .z = cube_pos.z + cube_size.z,
                                                  .w = 1.0f}),
            shz_xmtrx_transform_vec4((shz_vec4_t){.x = cube_pos.x + cube_size.x,
                                                  .y = cube_pos.y,
                                                  .z = cube_pos.z,
                                                  .w = 1.0f}),
            shz_xmtrx_transform_vec4((shz_vec4_t){.x = cube_pos.x + cube_size.x,
                                                  .y = cube_pos.y + cube_size.y,
                                                  .z = cube_pos.z,
                                                  .w = 1.0f}),
            shz_xmtrx_transform_vec4((shz_vec4_t){
                .x = cube_pos.x, .y = cube_pos.y, .z = cube_pos.z, .w = 1.0f}),
            shz_xmtrx_transform_vec4((shz_vec4_t){.x = cube_pos.x,
                                                  .y = cube_pos.y + cube_size.y,
                                                  .z = cube_pos.z,
                                                  .w = 1.0f})};
        for (int i = 0; i < 8; i++) {
          tverts[i].z = shz_invf_fsrra(tverts[i].w); // 1/w
          tverts[i].x *= tverts[i].z;
          tverts[i].y *= tverts[i].z;
        }
        // backface culling
        for (int side = 0; side < 6; side++) {
          shz_vec3_t cross = shz_vec3_cross(
              (shz_vec3_t){.x = tverts[cube_side_strips[side][1]].x -
                                tverts[cube_side_strips[side][0]].x,
                           .y = tverts[cube_side_strips[side][1]].y -
                                tverts[cube_side_strips[side][0]].y,
                           .z = tverts[cube_side_strips[side][1]].z -
                                tverts[cube_side_strips[side][0]].z},
              (shz_vec3_t){.x = tverts[cube_side_strips[side][2]].x -
                                tverts[cube_side_strips[side][0]].x,
                           .y = tverts[cube_side_strips[side][2]].y -
                                tverts[cube_side_strips[side][0]].y,
                           .z = tverts[cube_side_strips[side][2]].z -
                                tverts[cube_side_strips[side][0]].z});
          // printf("cross.z: %f\n", cross.z);
          if (cross.z > 0.0f) {
            continue;
          }
          draw_textured_sprite(tverts, side, &dr_state);
        }
      };
    }
  }
  pvr_dr_finish();
}

static inline void draw_sprite_line(shz_vec4_t *from, shz_vec4_t *to,
                                    float centerz, pvr_dr_state_t *dr_state) {
  pvr_sprite_col_t *quad = (pvr_sprite_col_t *)pvr_dr_target(*dr_state);
  quad->flags = PVR_CMD_VERTEX_EOL;
  if (from->x > to->x) {
    shz_vec4_t *tmp = from;
    from = to;
    to = tmp;
  }
  shz_vec3_t direction = shz_vec3_normalize(
      (shz_vec3_t){.e = {to->x - from->x, to->y - from->y, to->z - from->z}});
  quad->ax = from->x;
  quad->ay = from->y;
  quad->az = from->z + centerz * 0.1;
  quad->bx = to->x;
  quad->by = to->y;
  quad->bz = to->z + centerz * 0.1;
  quad->cx = to->x + LINE_WIDTH * XSCALE * direction.y;
  pvr_dr_commit(quad);
  quad = (pvr_sprite_col_t *)pvr_dr_target(*dr_state);
  pvr_sprite_col_t *quad2ndhalf = (pvr_sprite_col_t *)((int)quad - 32);
  quad2ndhalf->cy = to->y - LINE_WIDTH * direction.x;
  quad2ndhalf->cz = to->z + centerz * 0.1;
  quad2ndhalf->dx = from->x + LINE_WIDTH * XSCALE * direction.y;
  quad2ndhalf->dy = from->y - LINE_WIDTH * direction.x;
  pvr_dr_commit(quad);
}

void render_wire_grid(shz_vec4_t *min, shz_vec4_t *max, shz_vec4_t *dir1,
                      shz_vec4_t *dir2, int num_lines, uint32_t color,
                      pvr_dr_state_t *dr_state) {
  shz_vec4_t step = {
      .e = {shz_divf_fsrra((max->x - min->x), (num_lines + 1.0f)),
            shz_divf_fsrra((max->y - min->y), (num_lines + 1.0f)),
            shz_divf_fsrra((max->z - min->z), (num_lines + 1.0f)), 1.0f}};
  if (color != 0) {
    pvr_sprite_cxt_t cxt;
    pvr_sprite_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(*dr_state);
    pvr_sprite_compile(hdrpntr, &cxt);
    hdrpntr->argb = color;
    pvr_dr_commit(hdrpntr);
  }
  alignas(32) shz_vec4_t twolines[4] = {0};
  shz_vec4_t *from_v = twolines + 0;
  shz_vec4_t *to_v = twolines + 1;
  shz_vec4_t *from_h = twolines + 2;
  shz_vec4_t *to_h = twolines + 3;
  for (int i = 1; i <= num_lines; i++) {
    from_v->x = min->x + i * step.x * dir1->x;
    from_v->y = min->y + i * step.y * dir1->y;
    from_v->z = min->z + i * step.y * dir1->z;
    from_v->w = 1.0f;
    to_v->x = dir1->x == 0.0f ? max->x : min->x + i * step.x * dir1->x;
    to_v->y = dir1->y == 0.0f ? max->y : min->y + i * step.y * dir1->y;
    to_v->z = dir1->z == 0.0f ? max->z : min->z + i * step.z * dir1->z;
    to_v->w = 1.0f;
    from_h->x = min->x + i * step.x * dir2->x;
    from_h->y = min->y + i * step.y * dir2->y;
    from_h->z = min->z + i * step.z * dir2->z;
    from_h->w = 1.0f;
    to_h->x = dir2->x == 0.0f ? max->x : min->x + i * step.x * dir2->x;
    to_h->y = dir2->y == 0.0f ? max->y : min->y + i * step.y * dir2->y;
    to_h->z = dir2->z == 0.0f ? max->z : min->z + i * step.z * dir2->z;
    to_h->w = 1.0f;

    for (int i = 0; i < 4; i++) {
      twolines[i] = shz_xmtrx_transform_vec4(twolines[i]);
      twolines[i].z = shz_invf_fsrra(twolines[i].w); // 1/w
      twolines[i].x *= twolines[i].z;
      twolines[i].y *= twolines[i].z;
    }

    draw_sprite_line(from_v, to_v, 0, dr_state);
    draw_sprite_line(from_h, to_h, 0, dr_state);
  }
  draw_sprite_line(min, max, 0, dr_state);
}

void render_wire_cube(void) {
  set_cube_transform(1.0f);
  alignas(32) shz_vec4_t tverts[8] = {0};
  for (int i = 0; i < 8; i++) {
    tverts[i] = shz_xmtrx_transform_vec4(cube_vertices[i]);
    tverts[i].z = shz_invf_fsrra(tverts[i].w);
    tverts[i].x *= tverts[i].z;
    tverts[i].y *= tverts[i].z;
  }
  pvr_dr_state_t dr_state;
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_col(&cxt, PVR_LIST_OP_POLY);
  cxt.gen.culling = PVR_CULLING_NONE;
  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);
  for (int i = 0; i < 6; i++) {
    pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
    hdr.argb = cube_side_colors[i];
    *hdrpntr = hdr;
    pvr_dr_commit(hdrpntr);
    shz_vec4_t *ac = tverts + cube_side_strips[i][0];
    shz_vec4_t *bc = tverts + cube_side_strips[i][2];
    shz_vec4_t *cc = tverts + cube_side_strips[i][3];
    shz_vec4_t *dc = tverts + cube_side_strips[i][1];
    float centerz = (ac->z + bc->z + cc->z + dc->z) / 4.0f;
    draw_sprite_line(ac, dc, centerz, &dr_state);
    draw_sprite_line(bc, cc, centerz, &dr_state);
    draw_sprite_line(dc, cc, centerz, &dr_state);
    draw_sprite_line(ac, bc, centerz, &dr_state);
  }
  shz_vec4_t wiredir1 = (shz_vec4_t){.e = {1.0f, 0.0f, 0.0f, 1.0f}};
  shz_vec4_t wiredir2 = (shz_vec4_t){.e = {0.0f, 1.0f, 0.0f, 0.0f}};
  render_wire_grid(cube_vertices + 0, cube_vertices + 3, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[0], &dr_state);
  if (render_mode == WIREFRAME_FILLED) {
    for (uint32_t i = 1; i < cube_state.grid_size + 1; i++) {
      shz_vec4_t inner_from = *(cube_vertices + 0);
      shz_vec4_t inner_to = *(cube_vertices + 3);
      float z_offset = shz_divf_fsrra(i * ((inner_from.x - inner_to.x)), (cube_state.grid_size + 1));
      inner_from.z += z_offset;
      inner_to.z += z_offset;
      render_wire_grid(&inner_from, &inner_to, &wiredir1, &wiredir2,
                       cube_state.grid_size, 0x55FFFFFF, &dr_state);
    }
  }
  render_wire_grid(cube_vertices + 4, cube_vertices + 7, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[1], &dr_state);
  wiredir2.y = 0;
  wiredir2.z = 1;
  render_wire_grid(cube_vertices + 0, cube_vertices + 4, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[5], &dr_state);
  if (render_mode == WIREFRAME_FILLED) {
    for (uint32_t i = 1; i < cube_state.grid_size + 1; i++) {
      shz_vec4_t inner_from = *(cube_vertices + 0);
      shz_vec4_t inner_to = *(cube_vertices + 4);
      float y_offset = shz_divf_fsrra(i * ((inner_to.x - inner_from.x)), cube_state.grid_size + 1);
      inner_from.y += y_offset;
      inner_to.y += y_offset;
      render_wire_grid(&inner_from, &inner_to, &wiredir1, &wiredir2,
                       cube_state.grid_size, 0x55FFFFFF, &dr_state);
    }
  }
  render_wire_grid(cube_vertices + 1, cube_vertices + 5, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[4], &dr_state);
  wiredir1.x = 0;
  wiredir1.z = 1;
  wiredir2.z = 0;
  wiredir2.y = 1;
  render_wire_grid(cube_vertices + 4, cube_vertices + 3, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[3], &dr_state);
  render_wire_grid(cube_vertices + 6, cube_vertices + 1, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[2], &dr_state);
  pvr_dr_finish();
}

typedef struct __attribute__((packed)) {
  shz_vec3_t  normal;
  shz_vec3_t  v1;
  shz_vec3_t  v2;
  shz_vec3_t  v3;
  uint16_t    attrbytecount;
} stl_poly_t;

void render_teapot(void) {
  set_cube_transform(.30f);
  shz_xmtrx_apply_scale(0.1f * XSCALE, 0.1f, 0.1f);

  uint32_t num_polys = *((uint32_t*)(teapot_stl + 80));
  printf("Teapot polygons: %u, %u\n", num_polys, sizeof(stl_poly_t));
  stl_poly_t *polys = (stl_poly_t *)(teapot_stl + 84);

  pvr_dr_state_t dr_state;
  pvr_dr_init(&dr_state);

  pvr_poly_cxt_t cxt;
  pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
  cxt.gen.culling = PVR_CULLING_NONE;

  pvr_poly_hdr_t *hdrpntr = (pvr_poly_hdr_t *)pvr_dr_target(dr_state);
  pvr_poly_compile(hdrpntr, &cxt);
  pvr_dr_commit(hdrpntr);

  for (int p = 0; p < num_polys; p++) {

    // if (polys[p].attrbytecount != 0) {
    //   printf("Poly %d has attrbytecount %d, skipping\n", p,
    //   polys[p].attrbytecount);
    //   continue;
    // }
    // // if (p % 500 == 0){
    //   printf("Poly %d: v1=%.3f,%.3f,%.3f v2=%.3f,%.3f,%.3f v3=%.3f,%.3f,%.3f, %u\n", p,
    //         //  polys[p].normal.x, polys[p].normal.y, polys[p].normal.z,
    //          polys[p].v1.x, polys[p].v1.y, polys[p].v1.z,
    //          polys[p].v2.x, polys[p].v2.y, polys[p].v2.z,
    //          polys[p].v3.x, polys[p].v3.y, polys[p].v3.z,
    //          polys[p].attrbytecount
    //         );

    // // }
    shz_vec4_t v1 = shz_xmtrx_transform_vec4(
      (shz_vec4_t){
        .x = polys[p].v1.x, .y = polys[p].v1.y, .z = polys[p].v1.z, .w = 1.0f}
      );
    shz_vec4_t v2 = shz_xmtrx_transform_vec4((shz_vec4_t){
        .x = polys[p].v2.x, .y = polys[p].v2.y, .z = polys[p].v2.z, .w = 1.0f});
    shz_vec4_t v3 = shz_xmtrx_transform_vec4((shz_vec4_t){
        .x = polys[p].v3.x, .y = polys[p].v3.y, .z = polys[p].v3.z, .w = 1.0f});
    v1.z = shz_invf(v1.w);
    v1.x *= v1.z;
    v1.y *= v1.z;
    v2.z = shz_invf(v2.w);
    v2.x *= v2.z;
    v2.y *= v2.z;
    v3.z = shz_invf(v3.w);
    v3.x *= v3.z;
    v3.y *= v3.z;

    // backface culling
    shz_vec3_t cross = shz_vec3_cross(
        (shz_vec3_t){.x = v2.x - v1.x, .y = v2.y - v1.y, .z = v2.z - v1.z},
        (shz_vec3_t){.x = v3.x - v1.x, .y = v3.y - v1.y, .z = v3.z - v1.z});
    if (cross.z > 0.0f) {
      continue;
    }
    
    pvr_vertex_t *tri = (pvr_vertex_t *)pvr_dr_target(dr_state);
    tri->flags = PVR_CMD_VERTEX;
    tri->x = v1.x;
    tri->y = v1.y;
    tri->z = v1.z;
    tri->argb = 0xFFFFFFFF;
    pvr_dr_commit(tri);
    tri = (pvr_vertex_t *)pvr_dr_target(dr_state);
    tri->flags = PVR_CMD_VERTEX;
    tri->x = v2.x;
    tri->y = v2.y;
    tri->z = v2.z;
    tri->argb = 0xFFFFFFFF;
    pvr_dr_commit(tri);
    tri = (pvr_vertex_t *)pvr_dr_target(dr_state);
    tri->flags = PVR_CMD_VERTEX_EOL;
    tri->x = v3.x;
    tri->y = v3.y;
    tri->z = v3.z;
    tri->argb = 0xFFFFFFFF;
    pvr_dr_commit(tri);
  }
  pvr_dr_finish();
}


static inline void cube_reset_state() {
  uint32_t grid_size = cube_state.grid_size;
  cube_state = (struct cube){0};
  cube_state.grid_size = grid_size;
  fovy = DEFAULT_FOV;
  cube_state.pos.z = 12.0f;
  cube_state.rot.x = 0.85f * F_PI;
  cube_state.rot.y = 1.75f * F_PI;
  update_projection_view(fovy);
}

static inline int update_state() {
  for (int i = 0; i < 4; i++) {
    maple_device_t *cont = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
    if (cont) {
      cont_state_t *state = (cont_state_t *)maple_dev_status(cont);
      if (state->buttons & CONT_START) {
        return 0;
      }
      if (state->buttons & CONT_DPAD_RIGHT) {
        if ((dpad_right_down & (1 << i)) == 0) {
          dpad_right_down |= (1 << i);
          switch (render_mode) {
          case TEXTURED_TR:
          case CUBES_CUBE_MIN:
          case CUBES_CUBE_MAX:
            render_mode++;
            break;
          default:
            cube_state.grid_size += WIREFRAME_GRID_LINES_STEP;
            if (cube_state.grid_size > WIREFRAME_MAX_GRID_LINES) {
              cube_state.grid_size = WIREFRAME_MIN_GRID_LINES;
              render_mode++;
              if (render_mode >= MAX_RENDERMODE) {
                render_mode = TEXTURED_TR;
              }
            }
          }
        }
      } else {
        dpad_right_down &= ~(1 << i);
      }
      if (abs(state->joyx) > 16)
        cube_state.pos.x +=
            (state->joyx / 32768.0f) * 20.5f; // Increased sensitivity
      if (abs(state->joyy) > 16)
        cube_state.pos.y += (state->joyy / 32768.0f) *
                            20.5f; // Increased sensitivity and inverted Y
      if (state->ltrig > 16)       // Left trigger to zoom out
        cube_state.pos.z -= (state->ltrig / 255.0f) * ZOOM_SPEED;
      if (state->rtrig > 16) // Right trigger to zoom in
        cube_state.pos.z += (state->rtrig / 255.0f) * ZOOM_SPEED;
      if (cube_state.pos.z < MIN_ZOOM)
        cube_state.pos.z = MIN_ZOOM; // Farther away
      if (cube_state.pos.z > MAX_ZOOM)
        cube_state.pos.z = MAX_ZOOM; // Closer to the screen
      if (state->buttons & CONT_X)
        cube_state.speed.y += 0.001f;
      if (state->buttons & CONT_B)
        cube_state.speed.y -= 0.001f;
      if (state->buttons & CONT_A)
        cube_state.speed.x += 0.001f;
      if (state->buttons & CONT_Y)
        cube_state.speed.x -= 0.001f;
      if (state->buttons & CONT_DPAD_LEFT) {
        fovy = DEFAULT_FOV;
        cube_reset_state();
      }
      if (state->buttons & CONT_DPAD_DOWN) {
        fovy -= 1.0f;
        update_projection_view(fovy);
      }
      if (state->buttons & CONT_DPAD_UP) {
        fovy += 1.0f;
        update_projection_view(fovy);
      }
    }
  }
  cube_state.rot.x += cube_state.speed.x;
  cube_state.rot.y += cube_state.speed.y;
  cube_state.speed.x *= 0.99f;
  cube_state.speed.y *= 0.99f;
  return 1;
}

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

int main(int argc, char *argv[]) {

  printf("showframetimes: %d\n", SHOWFRAMETIMES);

#ifdef DEBUG
  gdb_init();
#endif
  pvr_set_bg_color(0.0, 0.0, 24.0f / 255.0f);
  pvr_init_params_t params = {
      {PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0,
       PVR_BINSIZE_8},
      3 << 19,       // Vertex buffer size, 3MB
      0,             // No DMA15
      SUPERSAMPLING, // Set horisontal FSAA
      0,             // Translucent Autosort enabled.
      3,             // Extra OPBs
      0,             // vbuf_doublebuf_disabled
  };
  pvr_init(&params);
  pvr_set_bg_color(0, 0, 0);
  if (!pvrtex_load_blob(&texture256_raw, &texture256x256))
    return -1;
  if (!pvrtex_load_blob(&texture128_raw, &texture128x128))
    return -1;
  // if (!pvrtex_load_palette_blob(palette64_raw, PVR_PAL_ARGB1555, 0))
  //   return -1;
  if (!pvrtex_load_blob(&texture32_raw, &texture32x32))
    return -1;
  if (!pvrtex_load_palette_blob(palette32_raw, PVR_PAL_RGB565, 256))
    return -1;

  cube_reset_state();

  while (update_state()) {
#if SHOWFRAMETIMES == 1
    vid_border_color(255, 0, 0);
#endif
    pvr_wait_ready();
#if SHOWFRAMETIMES == 1
    vid_border_color(0, 255, 0);
#endif
    pvr_scene_begin();
    switch (render_mode) {
    case TEXTURED_TR:
      pvr_list_begin(PVR_LIST_TR_POLY);
      render_txr_tr_cube();
      pvr_list_finish();
      break;
    case WIREFRAME_FILLED:
    case WIREFRAME_EMPTY:
      pvr_list_begin(PVR_LIST_OP_POLY);
      render_wire_cube();
      pvr_list_finish();
      break;
    case CUBES_CUBE_MAX:
      pvr_list_begin(PVR_LIST_OP_POLY);
      // render_cubes_cube();
      render_teapot();
      pvr_list_finish();
      break;
    case CUBES_CUBE_MIN:
      pvr_list_begin(PVR_LIST_PT_POLY);
      render_cubes_cube();
      pvr_list_finish();
      break;
    default:
      break;
    }
#if SHOWFRAMETIMES == 1
    vid_border_color(0, 0, 255);
#endif
    pvr_scene_finish();
  }
  printf("Cleaning up\n");
  pvrtex_unload(&texture256x256);
  pvrtex_unload(&texture128x128);
  pvrtex_unload(&texture32x32);
  pvr_shutdown(); // Clean up PVR resources
  vid_shutdown(); // This function reinitializes the video system to what
                  // dcload and friends expect it to be Run the main
                  // application here;
  printf("Exiting main\n");
  return 0;
}