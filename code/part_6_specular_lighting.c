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

#define SUPERSAMPLING 1  // Set to 1 to enable horizontal FSAA, 0 to disable
#if SUPERSAMPLING == 1
#define XSCALE 2
#else
#define XSCALE 1
#endif

#ifndef SHOWFRAMETIMES
#define SHOWFRAMETIMES 0
#endif

#include <sh4zam/shz_sh4zam.h>
#include <sh4zamsprites/cube.h> /* Cube vertices and side strips layout */
#include <sh4zamsprites/perspective.h> /* Perspective projection matrix functions */
#include <sh4zamsprites/shz_mdl.h>     /* sh4zam model loading and rendering */

#define DEFAULT_FOV 75.0f  // Field of view, adjust with dpad up/down
#define ZOOM_SPEED 0.3f
#define MIN_ZOOM -20.0f
#define MAX_ZOOM 15.0f
#define LINE_WIDTH 1.0f

static float fovy = DEFAULT_FOV;

static const alignas(32) uint8_t teapot_shzmdl[] = {
// #embed "../assets/models/teapot.stl"
// #embed "../assets/models/Utah_teapot_(solid).stl"
// #embed "../assets/models/teapot.shzmdl"
#embed "../assets/models/sphere.shzmdl"
};

static inline void draw_sprite_line(shz_vec4_t* from, shz_vec4_t* to,
                                    float centerz, pvr_dr_state_t* dr_state) {
    pvr_sprite_col_t* quad = (pvr_sprite_col_t*)pvr_dr_target(*dr_state);
    quad->flags = PVR_CMD_VERTEX_EOL;
    if (from->x > to->x) {
        shz_vec4_t* tmp = from;
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
    quad = (pvr_sprite_col_t*)pvr_dr_target(*dr_state);
    pvr_sprite_col_t* quad2ndhalf = (pvr_sprite_col_t*)((int)quad - 32);
    quad2ndhalf->cy = to->y - LINE_WIDTH * direction.x;
    quad2ndhalf->cz = to->z + centerz * 0.1;
    quad2ndhalf->dx = from->x + LINE_WIDTH * XSCALE * direction.y;
    quad2ndhalf->dy = from->y - LINE_WIDTH * direction.x;
    pvr_dr_commit(quad);
}

typedef struct __attribute__((packed)) {
    struct shz_mdl_tri_face_t;
    uint16_t attrbytecount;
} stl_poly_t;

static uint16_t light_rotation = 13337;
static uint16_t light_height = 4999;

static inline shz_vec3_t perspective_n_swizzle(shz_vec4_t v) {
    const float inv_w = shz_invf_fsrra(v.x);
    return shz_vec3_init(v.y * inv_w, v.z * inv_w, inv_w);
}

static inline float calc_light(shz_vec3_t* model_vert, shz_vec3_t* face_normal,
                               shz_vec3_t* light_pos,
                               shz_vec3_t* spec_light_pos,
                               shz_vec3_t* spec_view_pos,
                               shz_mat4x4_t* model_view,
                               shz_mat4x4_t* inverse_transpose) {
    shz_vec3_t diff_normal = shz_vec3_normalize(*face_normal);
    shz_vec3_t light_dir =
        shz_vec3_normalize(shz_vec3_sub(*light_pos, *model_vert));

    float light_intensity = SHZ_MAX(shz_vec3_dot(diff_normal, light_dir), 0.0f);

    // if (light_intensity > 0.0f) {
        /* specular light */
        shz_vec3_t spec_normal = shz_vec3_normalize(
            shz_mat4x4_trans_vec3(inverse_transpose, *face_normal));
        shz_vec3_t spec_vert_pos =
            shz_mat4x4_trans_vec3(model_view, *model_vert);
        shz_vec3_t spec_light_dir =
            shz_vec3_normalize(shz_vec3_sub(*spec_light_pos, spec_vert_pos));
        const float specular_strength = 1.5f;
        shz_vec3_t spec_view_dir =
            shz_vec3_normalize(shz_vec3_sub(*spec_view_pos, spec_vert_pos));
        shz_vec3_t reflect_dir =
            shz_vec3_reflect(shz_vec3_neg(spec_light_dir), spec_normal);
        const float dot_spec =
            SHZ_MAX(shz_vec3_dot(spec_view_dir, reflect_dir), 0.0f);
        light_intensity +=
            specular_strength * light_intensity * shz_powf(dot_spec, 32.0f);
    // }
    return SHZ_MAX(light_intensity, 0.0f);
}

void render_teapot(void) {
    const float screen_width = vid_mode->width * XSCALE;
    const float screen_height = vid_mode->height;
    const float near_z = 0.0f;
    const float fov = DEFAULT_FOV * SHZ_F_PI / 180.0f;
    const float aspect = shz_divf_fsrra(screen_width, (screen_height * XSCALE));

    shz_vec3_t eye = shz_vec3_init(0.0f, -0.00001f, 30.0f);
    shz_xmtrx_init_identity();
    kos_lookAt(eye, (shz_vec3_t){.e = {0.0f, 0.0f, 0.0f}},
               (shz_vec3_t){.e = {0.0f, 0.0f, 1.0f}});

    shz_xmtrx_translate(cube_state.pos.x, cube_state.pos.y - 10.0f,
                        cube_state.pos.z - 10.0f);
    shz_xmtrx_apply_rotation_x(cube_state.rot.x + SHZ_F_PI * 0.75f - 0.1f);
    shz_xmtrx_apply_rotation_y(cube_state.rot.y + SHZ_F_PI * 0.25f);

    shz_mat4x4_t model_view = {0};
    shz_mat4x4_t inverse_transpose = {0};
    shz_xmtrx_store_4x4(&model_view);
    shz_mat4x4_inverse(&model_view, &inverse_transpose);
    shz_mat4x4_transpose(&inverse_transpose, &inverse_transpose);

    shz_xmtrx_init_identity();
    shz_xmtrx_apply_permutation_wxyz();
    shz_xmtrx_apply_screen(screen_width, screen_height);
    shz_xmtrx_apply_perspective(fov, aspect, near_z);
    shz_xmtrx_apply_4x4(&model_view);

    pvr_dr_state_t dr_state;
    pvr_dr_init(&dr_state);

    light_rotation += 223;
    light_height += 127;
    const shz_sincos_t xy_rotation = shz_sincosu16(light_rotation);
    const shz_sincos_t height_variantion = shz_sincosu16(light_height);

    const float light_radius = 15.0f;

    shz_vec3_t light_color =
        shz_vec3_init(0.5f + (xy_rotation.cos + height_variantion.cos) * 0.25f,
                      0.5f + (xy_rotation.sin + height_variantion.sin) * 0.25f,
                      0.5f + (height_variantion.cos + xy_rotation.sin) * 0.25f);

    float high_attack_angle_x = SHZ_MAX(height_variantion.sin  * light_radius * 0.75f, 0.0f);
    float high_attack_angle_y = SHZ_MAX(height_variantion.cos  * light_radius * 0.75f, 0.0f);
    alignas(32) shz_vec3_t light_pos = {
        .x = xy_rotation.cos * light_radius - high_attack_angle_x,
        .y = xy_rotation.sin * light_radius - high_attack_angle_y,
        .z = -4.0f + light_radius + height_variantion.sin * light_radius};

#define LIGHT_CUBE_SIZE 0.33f

    alignas(32) shz_vec4_t light_quad[] = {
        {.e = {-LIGHT_CUBE_SIZE, -LIGHT_CUBE_SIZE, 0.0f, 1.0f}},
        {.e = {LIGHT_CUBE_SIZE, -LIGHT_CUBE_SIZE, 0.0f, 1.0f}},
        {.e = {LIGHT_CUBE_SIZE, LIGHT_CUBE_SIZE, 0.0f, 1.0f}},
        {.e = {-LIGHT_CUBE_SIZE, LIGHT_CUBE_SIZE, 0.0f, 1.0f}},
        {.e = {0.0f, 0.0f, 0.0f, 1.0f}},

    };
    for (int i = 0; i < 5; i++) {
        light_quad[i].xyz =
            perspective_n_swizzle(shz_xmtrx_transform_vec4((shz_vec4_t){
                .xyz = shz_vec3_add(light_quad[i].xyz, light_pos), .w = 1.0f}));
    }
    alignas(32) shz_vec4_t scene_center = (shz_vec4_t){
        .xyz = perspective_n_swizzle(shz_xmtrx_transform_vec4(
            (shz_vec4_t){.x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f})),
        .w = 1.0f};

    pvr_sprite_cxt_t spr_cxt;
    pvr_sprite_cxt_col(&spr_cxt, PVR_LIST_OP_POLY);
    spr_cxt.gen.culling = PVR_CULLING_NONE;
    pvr_sprite_hdr_t spr_hdr, *spr_hdr_pntr;
    pvr_sprite_compile(&spr_hdr, &spr_cxt);
    spr_hdr.argb = (uint32_t)(light_color.x * 255) << 16 |
                   (uint32_t)(light_color.y * 255) << 8 |
                   (uint32_t)(light_color.z * 255) | 0xFF000000;

    spr_hdr_pntr = (pvr_sprite_hdr_t*)pvr_dr_target(dr_state);
    *spr_hdr_pntr = spr_hdr;
    pvr_dr_commit(spr_hdr_pntr);
    draw_sprite_line(&((shz_vec4_t){.xyz = light_quad[4].xyz, .w = 1.0f}),
                     &scene_center, 0.0f, &dr_state);

    pvr_sprite_col_t* light = (pvr_sprite_col_t*)pvr_dr_target(dr_state);
    light->flags = PVR_CMD_VERTEX_EOL;
    light->ax = light_quad[0].x;
    light->ay = light_quad[0].y;
    light->az = light_quad[0].z;
    light->bx = light_quad[1].x;
    light->by = light_quad[1].y;
    light->bz = light_quad[1].z;
    light->cx = light_quad[2].x;
    pvr_dr_commit(light);
    light = (pvr_sprite_col_t*)pvr_dr_target(dr_state);
    pvr_sprite_col_t* light2ndhalf = (pvr_sprite_col_t*)((int)light - 32);
    light2ndhalf->cy = light_quad[3].y;
    light2ndhalf->cz = light_quad[3].z;
    light2ndhalf->dx = light_quad[3].x;
    light2ndhalf->dy = light_quad[3].y;
    pvr_dr_commit(light);

    shzmdl_hdr_t* shzmdl_hdr = (shzmdl_hdr_t*)(teapot_shzmdl);

    shz_mdl_tri_face_t* tris =
        (shz_mdl_tri_face_t*)((uint8_t*)teapot_shzmdl +
                               (shzmdl_hdr->offset.tri_faces << 5));
    shz_mdl_quad_face_t* quads =
        (shz_mdl_quad_face_t*)((uint8_t*)teapot_shzmdl +
                               (shzmdl_hdr->offset.quad_faces << 5));

    pvr_poly_cxt_t cxt;
    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.shading = PVR_SHADE_FLAT;
    cxt.gen.culling = PVR_CULLING_NONE;

    pvr_poly_hdr_t* hdrpntr = (pvr_poly_hdr_t*)pvr_dr_target(dr_state);
    // hdrpntr->cmd = PVR_CMD_USERCLIP;
    // hdrpntr->mode1 = 0;
    // hdrpntr->mode2 = 0;
    // hdrpntr->mode3 = 0;
    // hdrpntr->a = 0.0f;
    // hdrpntr->start_x = 0;
    // hdrpntr->start_y = 0;
    // hdrpntr->end_x = (vid_mode->width * XSCALE) >> 6;
    // hdrpntr->end_y = vid_mode->height >> 6;
    // pvr_dr_commit(hdrpntr);

    // hdrpntr = (pvr_poly_hdr_t*)pvr_dr_target(dr_state);
    pvr_poly_compile(hdrpntr, &cxt);
    // hdrpntr->m0.clip_mode = PVR_USERCLIP_INSIDE;
    // hdrpntr->m0.gouraud = PVR_SHADE_FLAT;
    pvr_dr_commit(hdrpntr);

    shz_vec3_t spec_light_pos = shz_mat4x4_trans_vec3(&model_view, light_pos);
    shz_vec3_t spec_view_pos = shz_mat4x4_trans_vec3(&model_view, eye);

    uint32_t fan_offset = shzmdl_hdr->offset.fans << 5;
    while (fan_offset) {
        shz_mdl_fan_t* cur_fan =
            (shz_mdl_fan_t*)((uint8_t*)&teapot_shzmdl + fan_offset);

        shz_mdl_vert_normal_t* fan_blades =
            (shz_mdl_vert_normal_t*)((uint8_t*)cur_fan + sizeof(shz_mdl_fan_t));

        shz_vec3_t fan_center = perspective_n_swizzle(shz_xmtrx_transform_vec4(
            (shz_vec4_t){.xyz = cur_fan->center, .w = 1.0f}));
        shz_vec3_t prev_left = perspective_n_swizzle(shz_xmtrx_transform_vec4(
            (shz_vec4_t){.xyz = (fan_blades + cur_fan->num_verts - 1)->vert,
                         .w = 1.0f}));

// #define FANSTRIPS
// #define FAN2TRIS
#define FAN2QUADS

        pvr_vertex_t* tri ;
#ifdef FANSTRIPS
        tri = (pvr_vertex_t*)pvr_dr_target(dr_state);
        tri->flags = PVR_CMD_VERTEX;
        tri->x = prev_left.x;
        tri->y = prev_left.y;
        tri->z = prev_left.z;
        pvr_dr_commit(tri);
        for (uint32_t f = 0; f < cur_fan->num_verts; f++) {
            /* ambient light */
            shz_vec3_t final_light =
                (shz_vec3_t){.x = 0.1f, .y = 0.1f, .z = 0.1f};

            /* diffuse and specular light */
            float light_intensity =
                calc_light(&(fan_blades + f)->vert, &(fan_blades + f)->normal,
                           &light_pos, &spec_light_pos, &spec_view_pos,
                           &model_view, &inverse_transpose);

            final_light = shz_vec3_add(
                final_light,
                (shz_vec3_t){.e = {light_intensity * light_color.x,
                                   light_intensity * light_color.y,
                                   light_intensity * light_color.z}});
            final_light = shz_vec3_clamp(final_light, 0.0f, 1.0f);
            uint32_t argb = (uint32_t)(final_light.x * 255) << 16 |
                            (uint32_t)(final_light.y * 255) << 8 |
                            (uint32_t)(final_light.z * 255) | 0xFF000000;

            shz_vec3_t cur_right =
                perspective_n_swizzle(shz_xmtrx_transform_vec4(
                    (shz_vec4_t){.xyz = (fan_blades + f)->vert, .w = 1.0f}));

            tri = (pvr_vertex_t*)pvr_dr_target(dr_state);
            tri->flags = PVR_CMD_VERTEX;
            tri->x = fan_center.x;
            tri->y = fan_center.y;
            tri->z = fan_center.z;
            pvr_dr_commit(tri);
            tri = (pvr_vertex_t*)pvr_dr_target(dr_state);
            tri->flags = PVR_CMD_VERTEX;
            tri->x = cur_right.x;
            tri->y = cur_right.y;
            tri->z = cur_right.z;
            tri->argb = argb;
            pvr_dr_commit(tri);
        }
                // finish the fan
        tri = (pvr_vertex_t*)pvr_dr_target(dr_state);
        tri->flags = PVR_CMD_VERTEX;
        tri->x = fan_center.x;
        tri->y = fan_center.y;
        tri->z = fan_center.z;
        pvr_dr_commit(tri);

        shz_vec3_t final_light = (shz_vec3_t){.x = 0.1f, .y = 0.1f, .z = 0.1f};

        /* diffuse and specular light */
        float light_intensity = calc_light(
            &fan_blades->vert, &fan_blades->normal, &light_pos, &spec_light_pos,
            &spec_view_pos, &model_view, &inverse_transpose);

        final_light = shz_vec3_add(
            final_light, (shz_vec3_t){.e = {light_intensity * light_color.x,
                                            light_intensity * light_color.y,
                                            light_intensity * light_color.z}});
        final_light = shz_vec3_clamp(final_light, 0.0f, 1.0f);
        uint32_t argb = (uint32_t)(final_light.x * 255) << 16 |
                        (uint32_t)(final_light.y * 255) << 8 |
                        (uint32_t)(final_light.z * 255) | 0xFF000000;

        shz_vec3_t cur_right = perspective_n_swizzle(shz_xmtrx_transform_vec4(
            (shz_vec4_t){.xyz = fan_blades->vert, .w = 1.0f}));

        tri = (pvr_vertex_t*)pvr_dr_target(dr_state);
        tri->flags = PVR_CMD_VERTEX_EOL;
        tri->x = cur_right.x;
        tri->y = cur_right.y;
        tri->z = cur_right.z;
        pvr_dr_commit(tri);
#endif
#ifdef FAN2TRIS
        for (uint32_t f = 0; f < cur_fan->num_verts; f++) {
            /* ambient light */
            shz_vec3_t final_light =
                (shz_vec3_t){.x = 0.1f, .y = 0.1f, .z = 0.1f};

            /* diffuse and specular light */
            float light_intensity =
                calc_light(&(fan_blades + f)->vert, &(fan_blades + f)->normal,
                           &light_pos, &spec_light_pos, &spec_view_pos,
                           &model_view, &inverse_transpose);

            final_light = shz_vec3_add(
                final_light,
                (shz_vec3_t){.e = {light_intensity * light_color.x,
                                   light_intensity * light_color.y,
                                   light_intensity * light_color.z}});
            final_light = shz_vec3_clamp(final_light, 0.0f, 1.0f);
            uint32_t argb = (uint32_t)(final_light.x * 255) << 16 |
                            (uint32_t)(final_light.y * 255) << 8 |
                            (uint32_t)(final_light.z * 255) | 0xFF000000;

            shz_vec3_t cur_right =
                perspective_n_swizzle(shz_xmtrx_transform_vec4(
                    (shz_vec4_t){.xyz = (fan_blades + f)->vert, .w = 1.0f}));

            tri = (pvr_vertex_t*)pvr_dr_target(dr_state);
            tri->flags = PVR_CMD_VERTEX;
            tri->x = fan_center.x;
            tri->y = fan_center.y;
            tri->z = fan_center.z;
            tri->argb = argb;
            pvr_dr_commit(tri);
            tri = (pvr_vertex_t*)pvr_dr_target(dr_state);
            tri->flags = PVR_CMD_VERTEX;
            tri->x = prev_left.x;
            tri->y = prev_left.y;
            tri->z = prev_left.z;
            tri->argb = argb;
            pvr_dr_commit(tri);
            tri = (pvr_vertex_t*)pvr_dr_target(dr_state);
            tri->flags = PVR_CMD_VERTEX_EOL;
            tri->x = cur_right.x;
            tri->y = cur_right.y;
            tri->z = cur_right.z;
            tri->argb = argb;
            pvr_dr_commit(tri);
            prev_left = cur_right;
        }
#endif

#ifdef FAN2QUADS
        for (uint32_t f = 0; f < cur_fan->num_verts; f+=2) {
            /* ambient light */
            shz_vec3_t final_light =
                (shz_vec3_t){.x = 0.1f, .y = 0.1f, .z = 0.1f};

            /* diffuse and specular light */
            float light_intensity =
                calc_light(&(fan_blades + f + 1)->vert, &(fan_blades + f)->normal,
                           &light_pos, &spec_light_pos, &spec_view_pos,
                           &model_view, &inverse_transpose);

            final_light = shz_vec3_add(
                final_light,
                (shz_vec3_t){.e = {light_intensity * light_color.x,
                                   light_intensity * light_color.y,
                                   light_intensity * light_color.z}});
            final_light = shz_vec3_clamp(final_light, 0.0f, 1.0f);

            spr_hdr.argb =  (uint32_t)(final_light.x * 255) << 16 |
                            (uint32_t)(final_light.y * 255) << 8 |
                            (uint32_t)(final_light.z * 255) | 0xFF000000;
            spr_hdr_pntr = (pvr_sprite_hdr_t*)pvr_dr_target(dr_state);
            *spr_hdr_pntr = spr_hdr;
            pvr_dr_commit(spr_hdr_pntr);

            shz_vec3_t cur_center =
                perspective_n_swizzle(shz_xmtrx_transform_vec4(
                    (shz_vec4_t){.xyz = (fan_blades + f)->vert, .w = 1.0f}));

            shz_vec3_t cur_right =
                perspective_n_swizzle(shz_xmtrx_transform_vec4(
                    (shz_vec4_t){.xyz = (fan_blades + f +1)->vert, .w = 1.0f}));
            pvr_sprite_col_t* fq = (pvr_sprite_col_t*)pvr_dr_target(dr_state);
            fq->flags = PVR_CMD_VERTEX_EOL;
            fq->ax = fan_center.x;
            fq->ay = fan_center.y;
            fq->az = fan_center.z;
            fq->bx = prev_left.x;
            fq->by = prev_left.y;
            fq->bz = prev_left.z;
            fq->cx = cur_center.x;
            pvr_dr_commit(fq);
            fq = (pvr_sprite_col_t*)pvr_dr_target(dr_state);
            pvr_sprite_col_t* fq2ndhalf = (pvr_sprite_col_t*)((int)fq - 32);
            fq2ndhalf->cy = cur_center.y;
            fq2ndhalf->cz = cur_center.z;
            fq2ndhalf->dx = cur_right.x;
            fq2ndhalf->dy = cur_right.y;
            pvr_dr_commit(fq);
            prev_left = cur_right;
        }
#endif

        fan_offset = cur_fan->next_fan_offset << 5;
    }

    
    for (int vidx = 0; vidx < shzmdl_hdr->num.tri_faces; vidx++) {
        shz_mdl_tri_face_t* triface = &tris[vidx];

        /* ambient light */
        shz_vec3_t final_light = (shz_vec3_t){.x = 0.1f, .y = 0.1f, .z = 0.1f};

        /* diffuse and specular light */
        float light_intensity = calc_light(
            &(triface->v1), &triface->normal, &light_pos, &spec_light_pos,
            &spec_view_pos, &model_view, &inverse_transpose);

        final_light = shz_vec3_add(
            final_light, (shz_vec3_t){.e = {light_intensity * light_color.x,
                                            light_intensity * light_color.y,
                                            light_intensity * light_color.z}});
        final_light = shz_vec3_clamp(final_light, 0.0f, 1.0f);

        uint32_t color = (uint32_t)(final_light.x * 255) << 16 |
                         (uint32_t)(final_light.y * 255) << 8 |
                         (uint32_t)(final_light.z * 255) | 0xFF000000;
        // color = (uint32_t)(triface->normal.x * 127 + 128) << 16 |
        //         (uint32_t)(triface->normal.y * 127 + 128) << 8 |
        //         (uint32_t)(triface->normal.z * 127 + 128) | 0xFF000000;
        
        alignas(32) shz_vec3_t v1 =
            perspective_n_swizzle(shz_xmtrx_transform_vec4(
                (shz_vec4_t){.xyz = triface->v1, .w = 1.0f}));
        alignas(32) shz_vec3_t v2 =
            perspective_n_swizzle(shz_xmtrx_transform_vec4(
                (shz_vec4_t){.xyz = triface->v2, .w = 1.0f}));
        alignas(32) shz_vec3_t v3 =
            perspective_n_swizzle(shz_xmtrx_transform_vec4(
                (shz_vec4_t){.xyz = triface->v3, .w = 1.0f}));
        pvr_vertex_t* v = (pvr_vertex_t*)pvr_dr_target(dr_state);
        v->flags = PVR_CMD_VERTEX;
        v->x = v1.x;
        v->y = v1.y;
        v->z = v1.z;
        pvr_dr_commit(v);
        v = (pvr_vertex_t*)pvr_dr_target(dr_state);
        v->flags = PVR_CMD_VERTEX;
        v->x = v2.x;
        v->y = v2.y;
        v->z = v2.z;
        pvr_dr_commit(v);
        v = (pvr_vertex_t*)pvr_dr_target(dr_state);
        v->flags = PVR_CMD_VERTEX_EOL;
        v->x = v3.x;
        v->y = v3.y;
        v->z = v3.z;
        v->argb = color;
        pvr_dr_commit(v);
    }

    for (int q = 0; q < shzmdl_hdr->num.quad_faces; q++) {
        shz_mdl_quad_face_t* quadface = &quads[q];

        /* ambient light */
        shz_vec3_t final_light = (shz_vec3_t){.x = 0.1f, .y = 0.1f, .z = 0.1f};

        /* diffuse and specular light */
        float light_intensity = calc_light(
            &(quadface->v1), &quadface->normal, &light_pos, &spec_light_pos,
            &spec_view_pos, &model_view, &inverse_transpose);

        final_light = shz_vec3_add(
            final_light, (shz_vec3_t){.e = {light_intensity * light_color.x,
                                            light_intensity * light_color.y,
                                            light_intensity * light_color.z}});
        final_light = shz_vec3_clamp(final_light, 0.0f, 1.0f);

        spr_hdr.m1.culling = PVR_CULLING_CW;
        spr_hdr.argb = (uint32_t)(final_light.x * 255) << 16 |
                       (uint32_t)(final_light.y * 255) << 8 |
                       (uint32_t)(final_light.z * 255) | 0xFF000000;

        // spr_hdr.argb = (uint32_t)(quadface->normal.x * 127 + 128) << 16 |
        //         (uint32_t)(quadface->normal.y * 127 + 128) << 8 |
        //         (uint32_t)(quadface->normal.z * 127 + 128) | 0xFF000000;

        // spr_hdr.m0.clip_mode = PVR_USERCLIP_INSIDE;

        spr_hdr_pntr = (pvr_sprite_hdr_t*)pvr_dr_target(dr_state);
        *spr_hdr_pntr = spr_hdr;
        pvr_dr_commit(spr_hdr_pntr);

        alignas(32) shz_vec3_t v1 =
            perspective_n_swizzle(shz_xmtrx_transform_vec4(
                (shz_vec4_t){.xyz = quadface->v1, .w = 1.0f}));
        alignas(32) shz_vec3_t v2 =
            perspective_n_swizzle(shz_xmtrx_transform_vec4(
                (shz_vec4_t){.xyz = quadface->v2, .w = 1.0f}));
        alignas(32) shz_vec3_t v3 =
            perspective_n_swizzle(shz_xmtrx_transform_vec4(
                (shz_vec4_t){.xyz = quadface->v3, .w = 1.0f}));
        alignas(32) shz_vec3_t v4 =
            perspective_n_swizzle(shz_xmtrx_transform_vec4(
                (shz_vec4_t){.xyz = quadface->v4, .w = 1.0f}));

        pvr_sprite_col_t* qface = (pvr_sprite_col_t*)pvr_dr_target(dr_state);
        qface->flags = PVR_CMD_VERTEX_EOL;
        qface->ax = v1.x;
        qface->ay = v1.y;
        qface->az = v1.z;
        qface->bx = v2.x;
        qface->by = v2.y;
        qface->bz = v2.z;
        qface->cx = v3.x;
        pvr_dr_commit(qface);
        qface = (pvr_sprite_col_t*)pvr_dr_target(dr_state);
        pvr_sprite_col_t* qface2ndhalf = (pvr_sprite_col_t*)((int)qface - 32);
        qface2ndhalf->cy = v3.y;
        qface2ndhalf->cz = v3.z;
        qface2ndhalf->dx = v4.x;
        qface2ndhalf->dy = v4.y;
        pvr_dr_commit(qface);
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
        maple_device_t* cont = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
        if (cont) {
            cont_state_t* state = (cont_state_t*)maple_dev_status(cont);
            if (state->buttons & CONT_START) {
                return 0;
            }
            if (abs(state->joyx) > 16)
                cube_state.pos.x +=
                    (state->joyx / 32768.0f) * 20.5f;  // Increased sensitivity
            if (abs(state->joyy) > 16)
                cube_state.pos.y +=
                    (state->joyy / 32768.0f) *
                    20.5f;          // Increased sensitivity and inverted Y
            if (state->ltrig > 16)  // Left trigger to zoom out
                cube_state.pos.z -= (state->ltrig / 255.0f) * ZOOM_SPEED;
            if (state->rtrig > 16)  // Right trigger to zoom in
                cube_state.pos.z += (state->rtrig / 255.0f) * ZOOM_SPEED;
            if (cube_state.pos.z < MIN_ZOOM)
                cube_state.pos.z = MIN_ZOOM;  // Farther away
            if (cube_state.pos.z > MAX_ZOOM)
                cube_state.pos.z = MAX_ZOOM;  // Closer to the screen
            if (state->buttons & CONT_X) cube_state.speed.y += 0.001f;
            if (state->buttons & CONT_B) cube_state.speed.y -= 0.001f;
            if (state->buttons & CONT_A) cube_state.speed.x += 0.001f;
            if (state->buttons & CONT_Y) cube_state.speed.x -= 0.001f;
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

int main(void) {
    printf("Starting main\n");
#ifdef DEBUG
    gdb_init();
#endif
    pvr_set_bg_color(0.0, 0.0, 24.0f / 255.0f);
    pvr_init_params_t params = {
        {PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0,
         PVR_BINSIZE_8},
        3 << 18,        // Vertex buffer size, 1.5MB
        0,              // No DMA15
        SUPERSAMPLING,  // Set horisontal FSAA
        0,              // Translucent Autosort enabled.
        3,              // Extra OPBs
        0,              // vbuf_doublebuf_disabled
    };
    vid_set_mode(DM_640x480, PM_RGB888P);
    pvr_set_bg_color(0, 0, 0);
    pvr_init(&params);
    PVR_SET(PVR_OBJECT_CLIP, 0.00001f);
    /** ensure that no NaNs or inf values persist in the xmtrx */
    shz_xmtrx_init_identity_safe();

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
        pvr_list_begin(PVR_LIST_OP_POLY);
        render_teapot();
        pvr_list_finish();
#if SHOWFRAMETIMES == 1
        vid_border_color(0, 0, 255);
#endif
        pvr_scene_finish();
    }
    printf("Cleaning up\n");
    pvr_shutdown();  // Clean up PVR resources
    vid_shutdown();  // This function reinitializes the video system to what
                     // dcload and friends expect it to be Run the main
                     // application here;
    printf("Exiting main\n");
    return 0;
}