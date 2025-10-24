#ifndef PERSPECTIVE_H
#define PERSPECTIVE_H

#include <dc/matrix.h> /* Matrix library headers for handling matrix operations */
#include <dc/matrix3d.h> /* Matrix3D library headers for handling 3D matrix operations */
#include <sh4zam/shz_sh4zam.h>
#include <stdio.h>

#ifndef XSCALE
#define XSCALE 1.0f
#endif

#define XCENTER 0.0f
#define YCENTER 0.0f
#define COT_FOVY_2 1.0f
#define ZNEAR 1.0f
#define ZFAR 100.0f

// clang-format off

/* Screen view matrix template (used to transform to screen space) */
static alignas(32) shz_mat4x4_t scrn_mtrx = {
    .elem2D = {
      { YCENTER, 0.0f,    0.0f, 0.0f},
      { 0.0f,    YCENTER, 0.0f, 0.0f},
      { 0.0f,    0.0f,    1.0f, 0.0f},
      { XCENTER, YCENTER, 0.0f, 1.0f}
    }
  };

/* Frustum matrix template (does perspective) */
static alignas(32) shz_mat4x4_t p_mtrx = {
  .elem2D = {
    { COT_FOVY_2, 0.0f,       0.0f,                            0.0f},
    { 0.0f,       COT_FOVY_2, 0.0f,                            0.0f},
    { 0.0f,       0.0f,       (ZFAR + ZNEAR) / (ZNEAR - ZFAR), -1.0f},
    { 0.0f,       0.0f,       2 * ZFAR *ZNEAR / (ZNEAR - ZFAR), 1.0f}
  }
};

// clang-format on

/**
 * Create and apply a perspective projection matrix to the current matrix
 * stack.
 * My backport of the mat_perspective function from KallistiOS's matrix3d.h
 *
 * \param screen_width The width of the screen, in pixels.
 * \param screen_height The height of the screen, in pixels.
 * \param fovy The field of view angle, in degrees, in the y direction.
 * \param near_z The distance to the near clipping plane.
 * \param far_z The distance to the far clipping plane.
 */
void kos_perspective(float screen_width, float screen_height, float fovy,
                     float near_z, float far_z) {
  scrn_mtrx.elem2D[0][0] = scrn_mtrx.elem2D[1][1] = scrn_mtrx.elem2D[3][1] =
      screen_height * 0.5f;
  scrn_mtrx.elem2D[3][0] = screen_width * 0.5f;
  shz_xmtrx_apply_4x4(&scrn_mtrx);

  float cot_fovy_2 = shz_invf_fsrra(shz_tanf(-(fovy * F_PI / 180.0f) * 0.5f));
  shz_mat4x4_set_scale(&p_mtrx, cot_fovy_2, cot_fovy_2,
                       shz_divf(far_z + near_z, near_z - far_z));
  p_mtrx.elem2D[3][2] = shz_divf(2.0f * far_z * near_z, near_z - far_z);

  shz_xmtrx_apply_4x4(&p_mtrx);
}


#define SHZ_IDENTITY_MAT4X4_TMPLATE                                            \
  {                                                                            \
    .elem2D = {                                                                \
      {1.0f, 0.0f, 0.0f, 0.0f},                                                \
      {0.0f, 1.0f, 0.0f, 0.0f},                                                \
      {0.0f, 0.0f, 1.0f, 0.0f},                                                \
      {0.0f, 0.0f, 0.0f, 1.0f}                                                 \
    }                                                                          \
  }

/* lookAt matrix template */
static alignas(32) shz_mat4x4_t lookAt_mtrx = SHZ_IDENTITY_MAT4X4_TMPLATE;

/* translation matrix template */
static alignas(32) shz_mat4x4_t translation_mtrx = SHZ_IDENTITY_MAT4X4_TMPLATE;


/**
 * Create and apply a lookAt matrix to the current matrix stack.
 * My backport of the mat_lookAt function from KallistiOS's matrix3d.h
 *
 * \param eye The position of the camera.
 * \param center The point the camera is looking at.
 * \param up The up vector.
 */
void kos_lookAt(const shz_vec3_t eye, const shz_vec3_t center,
  const shz_vec3_t up) {
  alignas(32) shz_vec3_t forward = shz_vec3_normalize(shz_vec3_sub(center, eye));
  lookAt_mtrx.left.xyz = shz_vec3_normalize(shz_vec3_cross(forward, up));
  lookAt_mtrx.up.xyz = shz_vec3_cross(lookAt_mtrx.left.xyz, forward);
  lookAt_mtrx.forward.xyz = shz_vec3_neg(forward);
  shz_xmtrx_apply_4x4(&lookAt_mtrx);

  translation_mtrx.pos.xyz = shz_vec3_neg(eye);
  shz_xmtrx_apply_4x4(&translation_mtrx);
}

alignas(32) shz_mat4x4_t stored_projection_view = {0};
void update_projection_view(float fovy) {
  shz_xmtrx_init_identity();
  kos_perspective(vid_mode->width * XSCALE, vid_mode->height, fovy, 0.f, -10.0f);

  kos_lookAt((shz_vec3_t){.e = {0.f, -0.00001f, 20.0f}},
             (shz_vec3_t){.e = {0.f, 0.f, 0.f}},
             (shz_vec3_t){.e = {0.f, 0.f, 1.f}});
  // shz_xmtrx_apply_lookat(
  //           (shz_vec3_t){0.f, -0.00001f, 20.0f},
  //           (shz_vec3_t){0.f, 0.f, 0.f},
  //           (shz_vec3_t){0.f, 0.f, 1.f});

  shz_xmtrx_store_4x4(&stored_projection_view);
}
#endif // PERSPECTIVE_H