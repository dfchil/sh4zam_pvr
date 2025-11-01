#include <assert.h>
#include <sh4zam/shz_sh4zam.h>
#include <stdio.h>

void print_matrix3x3(const char *label, shz_mat3x3_t *mtx) {

  printf("Matrix3x3 %s:\n", label);
  for (int r = 0; r < 3; r++) {
    printf("| ");
    for (int c = 0; c < 3; c++) {
      printf("%f ", mtx->elem2D[r][c]);
    }
    printf("|\n");
  }
}

void print_matrix4x4(const char *label, shz_mat4x4_t *mtx) {
  printf("Matrix4x4 %s:\n", label);
  for (int r = 0; r < 4; r++) {
    printf("| ");
    for (int c = 0; c < 4; c++) {
      printf("%f ", mtx->elem2D[r][c]);
    }
    printf("|\n");
  }
}

void print_xmtrx() {
  alignas(32) shz_mat4x4_t mtx = {0};
  shz_xmtrx_store_4x4(&mtx);
  printf("xmtrx -> ");
  print_matrix4x4("xmtrx", &mtx);
}

/**
 * Computes the transpose of a 3x3 matrix.
 */
SHZ_INLINE void shz_mat3x3_transpose(const shz_mat3x3_t *mtrx,
                                     shz_mat3x3_t *out) SHZ_NOEXCEPT {
  shz_xmtrx_load_transpose_3x3((const float *)mtrx);
  shz_xmtrx_store_3x3(out);
}

/**
 * Computes the inverse of a 3x3 matrix, saves cycles by not scaling by the
 * determinant, which makes sense when used for normals and lighting that are
 * usually normalized later.
 * 
 * mtrx: Pointer to the 3x3 matrix to invert.
 * out: Pointer to the resulting inverted matrix.
 *
 * \note
 * Only valid if the matrix is known to be orthonormal.
 */
SHZ_INLINE void shz_mat3x3_inverse_unscaled(const shz_mat3x3_t *mtrx,
                                            shz_mat3x3_t *out) SHZ_NOEXCEPT {
  // the transpose gives easy access to the rows as columns
  alignas(32) shz_mat3x3_t transpose;
  shz_mat3x3_transpose(mtrx, &transpose);

  out->col[0] = shz_vec3_cross(transpose.col[1], transpose.col[2]),
  out->col[1] = shz_vec3_cross(transpose.col[2], transpose.col[0]),
  out->col[2] = shz_vec3_cross(transpose.col[0], transpose.col[1]);
}

/**
 * Computes the inverse a 3x3 matrix
 *
 * mtrx: Pointer to the 3x3 matrix to invert.
 * out: Pointer to the resulting inverted matrix.
 *
 * \note
 * Only valid for non-singular matrices.
 */
SHZ_INLINE void shz_mat3x3_inverse(const shz_mat3x3_t *mtrx,
                                   shz_mat3x3_t *out) SHZ_NOEXCEPT {
  const float determinant =
      shz_vec3_dot(mtrx->col[0], shz_vec3_cross(mtrx->col[1], mtrx->col[2]));
  assert(determinant != 0.0f &&
         "shz_mat3x3_inverse: matrix is singular and cannot be inverted");
  const float inv_det = shz_invf(determinant);
  shz_mat3x3_inverse_unscaled(mtrx, out);
  for (int i = 0; i < 3; i++) {
    out->col[i] = shz_vec3_scale(out->col[i], inv_det);
  }
}

/**
 * Computes the inverse of a 4x4 matrix.
 * 
 * mtrx: Pointer to the 4x4 matrix to invert.
 * out: Pointer to the resulting inverted matrix.
 * 
 * \note
 * Only valid for non-singular matrices.
 * \note
 * In-place inversion (mtrx == out) is not supported.
 */
SHZ_INLINE void shz_mat4x4_inverse(const shz_mat4x4_t *mtrx,
                                   shz_mat4x4_t *out) SHZ_NOEXCEPT {
  assert(mtrx != out && "shz_mat4x4_inverse: in-place inversion not supported");
  /*
    If your matrix looks like this
      A = [ M   b ]
          [ 0   w ]

    where A is 4x4, M is 3x3, b is 3x1, and the bottom row is (0,0,0,w) with
    w != 0. For this block-triangular form, det(A) = det(M) * w. Then

      inv(A) = [ inv(M)        -inv(M) * b / w ]
               [   0                 1/w       ]

    Special case: if w == 1 (common for affine transforms), the top-right
    term reduces to -inv(M) * b.
  */
  if (mtrx->elem2D[3][0] == 0.0f && mtrx->elem2D[3][1] == 0.0f &&
      mtrx->elem2D[3][2] == 0.0f && mtrx->elem2D[3][3] != 0.0f) {
    alignas(32) shz_mat3x3_t invM;
    shz_mat3x3_inverse(&(shz_mat3x3_t){.col[0] = mtrx->col[0].xyz,
                                       .col[1] = mtrx->col[1].xyz,
                                       .col[2] = mtrx->col[2].xyz},
                       &invM);
    float inv_w = mtrx->elem2D[3][3];
    if (inv_w != 1.0f) {
      inv_w = shz_invf(inv_w);
    }
    out->col[0] = (shz_vec4_t){.xyz = invM.col[0], .w = 0.0f};
    out->col[1] = (shz_vec4_t){.xyz = invM.col[1], .w = 0.0f};
    out->col[2] = (shz_vec4_t){.xyz = invM.col[2], .w = 0.0f};
    out->col[3] = (shz_vec4_t){
        .xyz = inv_w == 1.0f ? shz_vec3_neg(shz_matrix3x3_trans_vec3(
                                   &invM, mtrx->col[3].xyz))
                             : shz_vec3_scale(shz_matrix3x3_trans_vec3(
                                                  &invM, mtrx->col[3].xyz),
                                              -inv_w),
        .w = inv_w};
    return;
  }
  // General case for full 4x4 matrix inversion, roughly ported from GLM
  const float sf00 = shz_fmaf(mtrx->elem2D[2][2], mtrx->elem2D[3][3],
                              -mtrx->elem2D[3][2] * mtrx->elem2D[2][3]);
  const float sf01 = shz_fmaf(mtrx->elem2D[2][1], mtrx->elem2D[3][3],
                              -mtrx->elem2D[3][1] * mtrx->elem2D[2][3]);
  const float sf02 = shz_fmaf(mtrx->elem2D[2][1], mtrx->elem2D[3][2],
                              -mtrx->elem2D[3][1] * mtrx->elem2D[2][2]);
  const float sf03 = shz_fmaf(mtrx->elem2D[2][0], mtrx->elem2D[3][3],
                              -mtrx->elem2D[3][0] * mtrx->elem2D[2][3]);
  const float sf04 = shz_fmaf(mtrx->elem2D[2][0], mtrx->elem2D[3][2],
                              -mtrx->elem2D[3][0] * mtrx->elem2D[2][2]);
  const float sf05 = shz_fmaf(mtrx->elem2D[2][0], mtrx->elem2D[3][1],
                              -mtrx->elem2D[3][0] * mtrx->elem2D[2][1]);
  const float sf06 = shz_fmaf(mtrx->elem2D[1][2], mtrx->elem2D[3][3],
                              -mtrx->elem2D[3][2] * mtrx->elem2D[1][3]);
  const float sf07 = shz_fmaf(mtrx->elem2D[1][1], mtrx->elem2D[3][3],
                              -mtrx->elem2D[3][1] * mtrx->elem2D[1][3]);
  const float sf08 = shz_fmaf(mtrx->elem2D[1][1], mtrx->elem2D[3][2],
                              -mtrx->elem2D[3][1] * mtrx->elem2D[1][2]);
  const float sf09 = shz_fmaf(mtrx->elem2D[1][0], mtrx->elem2D[3][3],
                              -mtrx->elem2D[3][0] * mtrx->elem2D[1][3]);
  const float sf10 = shz_fmaf(mtrx->elem2D[1][0], mtrx->elem2D[3][2],
                              -mtrx->elem2D[3][0] * mtrx->elem2D[1][2]);
  const float sf11 = shz_fmaf(mtrx->elem2D[1][0], mtrx->elem2D[3][1],
                              -mtrx->elem2D[3][0] * mtrx->elem2D[1][1]);
  const float sf12 = shz_fmaf(mtrx->elem2D[1][2], mtrx->elem2D[2][3],
                              -mtrx->elem2D[2][2] * mtrx->elem2D[1][3]);
  const float sf13 = shz_fmaf(mtrx->elem2D[1][1], mtrx->elem2D[2][3],
                              -mtrx->elem2D[2][1] * mtrx->elem2D[1][3]);
  const float sf14 = shz_fmaf(mtrx->elem2D[1][1], mtrx->elem2D[2][2],
                              -mtrx->elem2D[2][1] * mtrx->elem2D[1][2]);
  const float sf15 = shz_fmaf(mtrx->elem2D[1][0], mtrx->elem2D[2][3],
                              -mtrx->elem2D[2][0] * mtrx->elem2D[1][3]);
  const float sf16 = shz_fmaf(mtrx->elem2D[1][0], mtrx->elem2D[2][2],
                              -mtrx->elem2D[2][0] * mtrx->elem2D[1][2]);
  const float sf17 = shz_fmaf(mtrx->elem2D[1][0], mtrx->elem2D[2][1],
                              -mtrx->elem2D[2][0] * mtrx->elem2D[1][1]);

  out->col[0] = shz_vec4_init(
      +shz_fmaf(mtrx->elem2D[1][1], sf00,
                -shz_fmaf(mtrx->elem2D[1][2], sf01, mtrx->elem2D[1][3] * sf02)),
      -shz_fmaf(mtrx->elem2D[1][0], sf00,
                -shz_fmaf(mtrx->elem2D[1][2], sf03, mtrx->elem2D[1][3] * sf04)),
      +shz_fmaf(mtrx->elem2D[1][0], sf01,
                -shz_fmaf(mtrx->elem2D[1][1], sf03, mtrx->elem2D[1][3] * sf05)),
      -shz_fmaf(
          mtrx->elem2D[1][0], sf02,
          -shz_fmaf(mtrx->elem2D[1][1], sf04, mtrx->elem2D[1][2] * sf05)));

  out->col[1] = shz_vec4_init(
      -shz_fmaf(mtrx->elem2D[0][1], sf00,
                -shz_fmaf(mtrx->elem2D[0][2], sf01, mtrx->elem2D[0][3] * sf02)),
      +shz_fmaf(mtrx->elem2D[0][0], sf00,
                -shz_fmaf(mtrx->elem2D[0][2], sf03, mtrx->elem2D[0][3] * sf04)),
      -shz_fmaf(mtrx->elem2D[0][0], sf01,
                -shz_fmaf(mtrx->elem2D[0][1], sf03, mtrx->elem2D[0][3] * sf05)),
      +shz_fmaf(
          mtrx->elem2D[0][0], sf02,
          -shz_fmaf(mtrx->elem2D[0][1], sf04, mtrx->elem2D[0][2] * sf05)));
  out->col[2] = shz_vec4_init(
      +shz_fmaf(mtrx->elem2D[0][1], sf06,
                -shz_fmaf(mtrx->elem2D[0][2], sf07, mtrx->elem2D[0][3] * sf08)),
      -shz_fmaf(mtrx->elem2D[0][0], sf06,
                -shz_fmaf(mtrx->elem2D[0][2], sf09, mtrx->elem2D[0][3] * sf10)),
      +shz_fmaf(mtrx->elem2D[0][0], sf07,
                -shz_fmaf(mtrx->elem2D[0][1], sf09, mtrx->elem2D[0][3] * sf11)),
      -shz_fmaf(
          mtrx->elem2D[0][0], sf08,
          -shz_fmaf(mtrx->elem2D[0][1], sf10, mtrx->elem2D[0][2] * sf11)));
  out->col[3] = shz_vec4_init(
      -shz_fmaf(mtrx->elem2D[0][1], sf12,
                -shz_fmaf(mtrx->elem2D[0][2], sf13, mtrx->elem2D[0][3] * sf14)),
      +shz_fmaf(mtrx->elem2D[0][0], sf12,
                -shz_fmaf(mtrx->elem2D[0][2], sf15, mtrx->elem2D[0][3] * sf16)),
      -shz_fmaf(mtrx->elem2D[0][0], sf13,
                -shz_fmaf(mtrx->elem2D[0][1], sf15, mtrx->elem2D[0][3] * sf17)),
      +shz_fmaf(
          mtrx->elem2D[0][0], sf14,
          -shz_fmaf(mtrx->elem2D[0][1], sf16, mtrx->elem2D[0][2] * sf17)));

  float inv_det = shz_invf(
      shz_fmaf(mtrx->elem2D[0][0], out->elem2D[0][0],
               shz_fmaf(mtrx->elem2D[0][1], out->elem2D[0][1],
                        shz_fmaf(mtrx->elem2D[0][2], out->elem2D[0][2],
                                 mtrx->elem2D[0][3] * out->elem2D[0][3]))));

  for (int c = 0; c < 4; c++) {
    out->col[c] = shz_vec4_scale(out->col[c], inv_det);
  }
}