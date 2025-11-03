#ifndef CODE_MAT_INVERSE_H
#define CODE_MAT_INVERSE_H


#include <sh4zam/shz_sh4zam.h>

void print_mat4x4(const char* label, shz_mat4x4_t* mtx);

void print_mat3x3(const char* label, shz_mat3x3_t* mtx);

void print_xmtrx();

/**
 * Computes the inverse of a 4x4 matrix.
 *
 * mtrx: Pointer to the 4x4 matrix to invert.
 * out: Pointer to the resulting inverted matrix.
 *
 * \note
 * In-place inversion (mtrx == out) is not supported.
 */
void shz_mat4x4_inverse(const shz_mat4x4_t* mtrx, shz_mat4x4_t* out);

/**
 * Computes the inverse of a 3x3 matrix.
 *
 * mtrx: Pointer to the 3x3 matrix to invert.
 * out: Pointer to the resulting inverted matrix.
 *
 * \note
 * Only valid for non-singular matrices.
 */
void shz_mat3x3_inverse(const shz_mat3x3_t* mtrx, shz_mat3x3_t* out);

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
void shz_mat3x3_inverse_unscaled(const shz_mat3x3_t* mtrx, shz_mat3x3_t* out);

/**
 * Computes the transpose of a 3x3 matrix.
 *
 * mtrx: Pointer to the 3x3 matrix to transpose.
 * out: Pointer to the resulting transposed matrix.
 */
void shz_mat3x3_transpose(const shz_mat3x3_t* mtrx, shz_mat3x3_t* out);
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
void shz_mat3x3_inverse_unscaled(const shz_mat3x3_t* mtrx, shz_mat3x3_t* out);



void shz_mat4x4_inverse_from_glm(const shz_mat4x4_t *mtrx,
  shz_mat4x4_t *out);

#endif  // CODE_MAT_INVERSE_H
