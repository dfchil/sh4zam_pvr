#include <sh4zam/shz_sh4zam.h>

/**
 * Computes the inverse of the upper-left 3x3 portion of the current
 * transformation matrix, saves cycles by not scaling with the determinant,
 * which makes sense when used for normals and lighting that are usually
 * normalized afterwards.
 *
 * \note
 * Only valid if the matrix is known to be orthonormal.
 */
shz_mat3x3_t shz_xmtrx_upper_inverse_unscaled() {
  alignas(32) shz_mat3x3_t topleft = {0};

  // by storing the transpose, we get easy access to the rows as columns
  shz_xmtrx_store_transpose_3x3(&topleft);

  // the above are the rows of the inverse matrix, so set them as columns in
  // the output
  return (shz_mat3x3_t) {
    .col[0] = shz_vec3_cross(topleft.col[1], topleft.col[2]),
    .col[1] = shz_vec3_cross(topleft.col[2], topleft.col[0]),
    .col[2] = shz_vec3_cross(topleft.col[0], topleft.col[1])
  };
}

/**
 * Computes the inverse of the upper-left 3x3 portion of the current
 * transformation matrix.
 * 
 * \note
 * Only valid for non-singular matrices.
 */
shz_mat3x3_t shz_xmtrx_upper_inverse() {
  alignas(32) const shz_mat3x3_t topleft = {0};

  // by storing the transpose, we get easy access to the rows as columns
  shz_xmtrx_store_transpose_3x3(&topleft);
  float inv_det = shz_invf(shz_vec3_dot(
      topleft.col[0], shz_vec3_cross(topleft.col[1], topleft.col[2])));

  assert(inv_det != 0.0f &&
         "shz_xmtrx_upper_inverse: matrix is singular and cannot be inverted");

  // the above are the rows of the inverse matrix, so set them as columns in the
  // output
  return (shz_mat3x3_t){
      .col[0] = shz_vec3_scale(shz_vec3_cross(topleft.col[1], topleft.col[2]),
                               inv_det),
      .col[1] = shz_vec3_scale(shz_vec3_cross(topleft.col[2], topleft.col[0]),
                               inv_det),
      .col[2] = shz_vec3_scale(shz_vec3_cross(topleft.col[0], topleft.col[1]),
                               inv_det)};
}

/**
 * Computes the inverse of the current 4x4 transformation matrix.
 * \note
 * Only valid for non-singular matrices.
 */
shz_mat4x4_t shz_xmtrx_inverse() {
  /*
    If your matrix looks like this

    A = [ M   b  ]
    [ 0   1  ]

    where A is 4x4, M is 3x3, b is 3x1, and the bottom row is (0,0,0,1), then
    inv(A) =  [ inv(M)   -inv(M) * b ]
              [   0            1     ].  */
  alignas(32) const shz_vec4_t r3 = shz_xmtrx_read_row(3);
  if (r3.e[0] == 0.0f && r3.e[1] == 0.0f && r3.e[2] == 0.0f &&
      r3.e[3] == 1.0f) {
    alignas(32) const shz_mat3x3_t M = shz_xmtrx_upper_inverse();
    return (shz_mat4x4_t){
        .col[0] = (shz_vec4_t){.xyz = M.col[0], .w = 0.0f},
        .col[1] = (shz_vec4_t){.xyz = M.col[1], .w = 0.0f},
        .col[2] = (shz_vec4_t){.xyz = M.col[2], .w = 0.0f},
        .col[3] = (shz_vec4_t){.xyz = shz_vec3_neg(shz_matrix3x3_trans_vec3(
                                   &M, shz_xmtrx_read_col(3).xyz)),
                               .w = 1.0f}};
  }
  // General case for full 4x4 matrix inversion, roughly ported from GLM
  alignas(32) shz_mat4x4_t mtrx = {0};
  shz_xmtrx_store_4x4(&mtrx);

  const float sfac00 = shz_fmaf(mtrx.elem2D[2][2], mtrx.elem2D[3][3],
                                -mtrx.elem2D[3][2] * mtrx.elem2D[2][3]);
  const float sfac01 = shz_fmaf(mtrx.elem2D[2][1], mtrx.elem2D[3][3],
                                -mtrx.elem2D[3][1] * mtrx.elem2D[2][3]);
  const float sfac02 = shz_fmaf(mtrx.elem2D[2][1], mtrx.elem2D[3][2],
                                -mtrx.elem2D[3][1] * mtrx.elem2D[2][2]);
  const float sfac03 = shz_fmaf(mtrx.elem2D[2][0], mtrx.elem2D[3][3],
                                -mtrx.elem2D[3][0] * mtrx.elem2D[2][3]);
  const float sfac04 = shz_fmaf(mtrx.elem2D[2][0], mtrx.elem2D[3][2],
                                -mtrx.elem2D[3][0] * mtrx.elem2D[2][2]);
  const float sfac05 = shz_fmaf(mtrx.elem2D[2][0], mtrx.elem2D[3][1],
                                -mtrx.elem2D[3][0] * mtrx.elem2D[2][1]);
  const float sfac06 = shz_fmaf(mtrx.elem2D[1][2], mtrx.elem2D[3][3],
                                -mtrx.elem2D[3][2] * mtrx.elem2D[1][3]);
  const float sfac07 = shz_fmaf(mtrx.elem2D[1][1], mtrx.elem2D[3][3],
                                -mtrx.elem2D[3][1] * mtrx.elem2D[1][3]);
  const float sfac08 = shz_fmaf(mtrx.elem2D[1][1], mtrx.elem2D[3][2],
                                -mtrx.elem2D[3][1] * mtrx.elem2D[1][2]);
  const float sfac09 = shz_fmaf(mtrx.elem2D[1][0], mtrx.elem2D[3][3],
                                -mtrx.elem2D[3][0] * mtrx.elem2D[1][3]);
  const float sfac10 = shz_fmaf(mtrx.elem2D[1][0], mtrx.elem2D[3][2],
                                -mtrx.elem2D[3][0] * mtrx.elem2D[1][2]);
  const float sfac11 = shz_fmaf(mtrx.elem2D[1][0], mtrx.elem2D[3][1],
                                -mtrx.elem2D[3][0] * mtrx.elem2D[1][1]);
  const float sfac12 = shz_fmaf(mtrx.elem2D[1][2], mtrx.elem2D[2][3],
                                -mtrx.elem2D[2][2] * mtrx.elem2D[1][3]);
  const float sfac13 = shz_fmaf(mtrx.elem2D[1][1], mtrx.elem2D[2][3],
                                -mtrx.elem2D[2][1] * mtrx.elem2D[1][3]);
  const float sfac14 = shz_fmaf(mtrx.elem2D[1][1], mtrx.elem2D[2][2],
                                -mtrx.elem2D[2][1] * mtrx.elem2D[1][2]);
  const float sfac15 = shz_fmaf(mtrx.elem2D[1][0], mtrx.elem2D[2][3],
                                -mtrx.elem2D[2][0] * mtrx.elem2D[1][3]);
  const float sfac16 = shz_fmaf(mtrx.elem2D[1][0], mtrx.elem2D[2][2],
                                -mtrx.elem2D[2][0] * mtrx.elem2D[1][2]);
  const float sfac17 = shz_fmaf(mtrx.elem2D[1][0], mtrx.elem2D[2][1],
                                -mtrx.elem2D[2][0] * mtrx.elem2D[1][1]);

  alignas(32) shz_mat4x4_t inverted = {
      .elem2D = {{+shz_fmaf(mtrx.elem2D[1][1], sfac00,
                            -shz_fmaf(mtrx.elem2D[1][2], sfac01,
                                      mtrx.elem2D[1][3] * sfac02)),
                  -shz_fmaf(mtrx.elem2D[1][0], sfac00,
                            -shz_fmaf(mtrx.elem2D[1][2], sfac03,
                                      mtrx.elem2D[1][3] * sfac04)),
                  +shz_fmaf(mtrx.elem2D[1][0], sfac01,
                            -shz_fmaf(mtrx.elem2D[1][1], sfac03,
                                      mtrx.elem2D[1][3] * sfac05)),
                  -shz_fmaf(mtrx.elem2D[1][0], sfac02,
                            -shz_fmaf(mtrx.elem2D[1][1], sfac04,
                                      mtrx.elem2D[1][2] * sfac05))},
                 {
                     -shz_fmaf(mtrx.elem2D[0][1], sfac00,
                               -shz_fmaf(mtrx.elem2D[0][2], sfac01,
                                         mtrx.elem2D[0][3] * sfac02)),
                     +shz_fmaf(mtrx.elem2D[0][0], sfac00,
                               -shz_fmaf(mtrx.elem2D[0][2], sfac03,
                                         mtrx.elem2D[0][3] * sfac04)),
                     -shz_fmaf(mtrx.elem2D[0][0], sfac01,
                               -shz_fmaf(mtrx.elem2D[0][1], sfac03,
                                         mtrx.elem2D[0][3] * sfac05)),
                     +shz_fmaf(mtrx.elem2D[0][0], sfac02,
                               -shz_fmaf(mtrx.elem2D[0][1], sfac04,
                                         mtrx.elem2D[0][2] * sfac05)),
                 },
                 {+shz_fmaf(mtrx.elem2D[0][1], sfac06,
                            -shz_fmaf(mtrx.elem2D[0][2], sfac07,
                                      mtrx.elem2D[0][3] * sfac08)),
                  -shz_fmaf(mtrx.elem2D[0][0], sfac06,
                            -shz_fmaf(mtrx.elem2D[0][2], sfac09,
                                      mtrx.elem2D[0][3] * sfac10)),
                  +shz_fmaf(mtrx.elem2D[0][0], sfac07,
                            -shz_fmaf(mtrx.elem2D[0][1], sfac09,
                                      mtrx.elem2D[0][3] * sfac11)),
                  -shz_fmaf(mtrx.elem2D[0][0], sfac08,
                            -shz_fmaf(mtrx.elem2D[0][1], sfac10,
                                      mtrx.elem2D[0][2] * sfac11))},

                 {-shz_fmaf(mtrx.elem2D[0][1], sfac12,
                            -shz_fmaf(mtrx.elem2D[0][2], sfac13,
                                      mtrx.elem2D[0][3] * sfac14)),
                  +shz_fmaf(mtrx.elem2D[0][0], sfac12,
                            -shz_fmaf(mtrx.elem2D[0][2], sfac15,
                                      mtrx.elem2D[0][3] * sfac16)),
                  -shz_fmaf(mtrx.elem2D[0][0], sfac13,
                            -shz_fmaf(mtrx.elem2D[0][1], sfac15,
                                      mtrx.elem2D[0][3] * sfac17)),
                  +shz_fmaf(mtrx.elem2D[0][0], sfac14,
                            -shz_fmaf(mtrx.elem2D[0][1], sfac16,
                                      mtrx.elem2D[0][2] * sfac17))}}};

  const float inv_det = shz_invf(
      shz_fmaf(mtrx.elem2D[0][0], inverted.elem2D[0][0],
               shz_fmaf(mtrx.elem2D[0][1], inverted.elem2D[0][1],
                        shz_fmaf(mtrx.elem2D[0][2], inverted.elem2D[0][2],
                                 mtrx.elem2D[0][3] * inverted.elem2D[0][3]))));

  for (int c = 0; c < 4; c++) {
    inverted.col[c] = shz_vec4_scale(inverted.col[c], inv_det);
  }

  return inverted;
}