#include <assert.h>
#include <sh4zamsprites/mat_inverse.h>
#include <stdio.h>

void print_mat3x3(const char* label, shz_mat3x3_t* mtx) {
    printf("Matrix3x3 %s:\n", label);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            printf(" |%12.4f", mtx->elem2D[c][r]);
        }
        printf("|\n");
    }
}

void print_mat4x4(const char* label, shz_mat4x4_t* mtx) {
    printf("Matrix4x4 %s:\n", label);
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            printf(" |%12.4f ", mtx->elem2D[c][r]);
        }
        printf("|\n");
    }
}

void print_xmtrx() {
    alignas(32) shz_mat4x4_t mtx = {0};
    shz_xmtrx_store_4x4(&mtx);
    printf("xmtrx -> ");
    print_mat4x4("xmtrx", &mtx);
}

void shz_mat3x3_transpose(const shz_mat3x3_t* mtrx,
                          shz_mat3x3_t* out) SHZ_NOEXCEPT {
    shz_xmtrx_load_transpose_3x3((const float*)mtrx);
    shz_xmtrx_store_3x3(out);
}

void shz_mat3x3_inverse_unscaled(const shz_mat3x3_t* mtrx,
                                 shz_mat3x3_t* out) SHZ_NOEXCEPT {
    // the transpose gives easy access to the rows as columns
    alignas(32) shz_mat3x3_t transpose;
    shz_mat3x3_transpose(mtrx, &transpose);

    out->col[0] = shz_vec3_cross(transpose.col[1], transpose.col[2]),
    out->col[1] = shz_vec3_cross(transpose.col[2], transpose.col[0]),
    out->col[2] = shz_vec3_cross(transpose.col[0], transpose.col[1]);
}

void shz_mat3x3_inverse(const shz_mat3x3_t* mtrx,
                        shz_mat3x3_t* out) SHZ_NOEXCEPT {
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

void shz_mat4x4_inverse(const shz_mat4x4_t* mtrx,
                        shz_mat4x4_t* out) SHZ_NOEXCEPT {
    /**
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
    if (mtrx->col[0].w == 0.0f && mtrx->col[1].w == 0.0f &&
        mtrx->col[2].w == 0.0f && mtrx->col[3].w != 0.0f) {
        alignas(32) shz_mat3x3_t invM;
        shz_mat3x3_inverse(&(shz_mat3x3_t){.col[0] = mtrx->col[0].xyz,
                                           .col[1] = mtrx->col[1].xyz,
                                           .col[2] = mtrx->col[2].xyz},
                           &invM);
        float inv_w = mtrx->col[3].w;
        if (inv_w != 1.0f) {
            inv_w = shz_invf(inv_w);
        }
        out->col[0] = (shz_vec4_t){.xyz = invM.col[0], .w = 0.0f};
        out->col[1] = (shz_vec4_t){.xyz = invM.col[1], .w = 0.0f};
        out->col[2] = (shz_vec4_t){.xyz = invM.col[2], .w = 0.0f};
        out->col[3] = (shz_vec4_t){
            .xyz = shz_vec3_scale(
                shz_matrix3x3_trans_vec3(&invM, mtrx->col[3].xyz), -inv_w),
            .w = inv_w};
        return;
    }
    // General case for full 4x4 matrix inversion, roughly ported from cglm
    const float m00 = mtrx->elem2D[0][0], m01 = mtrx->elem2D[0][1],
                m02 = mtrx->elem2D[0][2], m03 = mtrx->elem2D[0][3],
                m10 = mtrx->elem2D[1][0], m11 = mtrx->elem2D[1][1],
                m12 = mtrx->elem2D[1][2], m13 = mtrx->elem2D[1][3],
                m20 = mtrx->elem2D[2][0], m21 = mtrx->elem2D[2][1],
                m22 = mtrx->elem2D[2][2], m23 = mtrx->elem2D[2][3],
                m30 = mtrx->elem2D[3][0], m31 = mtrx->elem2D[3][1],
                m32 = mtrx->elem2D[3][2], m33 = mtrx->elem2D[3][3],

                c1 = shz_fmaf(m22, m33, -m23 * m32),
                c2 = shz_fmaf(m02, m13, -m03 * m12),
                c3 = shz_fmaf(m20, m33, -m23 * m30),
                c4 = shz_fmaf(m00, m13, -m03 * m10),
                c5 = shz_fmaf(m21, m33, -m23 * m31),
                c6 = shz_fmaf(m01, m13, -m03 * m11),
                c7 = shz_fmaf(m20, m31, -m21 * m30),
                c8 = shz_fmaf(m00, m11, -m01 * m10),
                c9 = shz_fmaf(m21, m32, -m22 * m31),
                c10 = shz_fmaf(m01, m12, -m02 * m11),
                c11 = shz_fmaf(m20, m32, -m22 * m30),
                c12 = shz_fmaf(m00, m12, -m02 * m10),
                inv_det = shz_invf(c8 * c1 + c4 * c9 + c10 * c3 + c2 * c7 -
                                   c12 * c5 - c6 * c11);

    out->elem2D[0][0] = +(m11 * c1 - m12 * c5 + m13 * c9) * inv_det;
    out->elem2D[0][1] = -(m01 * c1 - m02 * c5 + m03 * c9) * inv_det;
    out->elem2D[0][2] = +(m31 * c2 - m32 * c6 + m33 * c10) * inv_det;
    out->elem2D[0][3] = -(m21 * c2 - m22 * c6 + m23 * c10) * inv_det;

    out->elem2D[1][0] = -(m10 * c1 - m12 * c3 + m13 * c11) * inv_det;
    out->elem2D[1][1] = +(m00 * c1 - m02 * c3 + m03 * c11) * inv_det;
    out->elem2D[1][2] = -(m30 * c2 - m32 * c4 + m33 * c12) * inv_det;
    out->elem2D[1][3] = +(m20 * c2 - m22 * c4 + m23 * c12) * inv_det;

    out->elem2D[2][0] = +(m10 * c5 - m11 * c3 + m13 * c7) * inv_det;
    out->elem2D[2][1] = -(m00 * c5 - m01 * c3 + m03 * c7) * inv_det;
    out->elem2D[2][2] = +(m30 * c6 - m31 * c4 + m33 * c8) * inv_det;
    out->elem2D[2][3] = -(m20 * c6 - m21 * c4 + m23 * c8) * inv_det;

    out->elem2D[3][0] = -(m10 * c9 - m11 * c11 + m12 * c7) * inv_det;
    out->elem2D[3][1] = +(m00 * c9 - m01 * c11 + m02 * c7) * inv_det;
    out->elem2D[3][2] = -(m30 * c10 - m31 * c12 + m32 * c8) * inv_det;
    out->elem2D[3][3] = +(m20 * c10 - m21 * c12 + m22 * c8) * inv_det;
}

void shz_mat4x4_inverse_from_glm(const shz_mat4x4_t* mtrx,
                                 shz_mat4x4_t* out) SHZ_NOEXCEPT {
    const float coef00 = shz_fmaf(mtrx->elem2D[2][2], mtrx->elem2D[3][3],
                                  -mtrx->elem2D[2][3] * mtrx->elem2D[3][2]);
    const float coef02 = shz_fmaf(mtrx->elem2D[2][1], mtrx->elem2D[3][3],
                                  -mtrx->elem2D[2][3] * mtrx->elem2D[3][1]);
    const float coef03 = shz_fmaf(mtrx->elem2D[2][1], mtrx->elem2D[3][2],
                                  -mtrx->elem2D[2][2] * mtrx->elem2D[3][1]);

    const float coef04 = shz_fmaf(mtrx->elem2D[1][2], mtrx->elem2D[3][3],
                                  -mtrx->elem2D[1][3] * mtrx->elem2D[3][2]);
    const float coef06 = shz_fmaf(mtrx->elem2D[1][1], mtrx->elem2D[3][3],
                                  -mtrx->elem2D[1][3] * mtrx->elem2D[3][1]);
    const float coef07 = shz_fmaf(mtrx->elem2D[1][1], mtrx->elem2D[3][2],
                                  -mtrx->elem2D[1][2] * mtrx->elem2D[3][1]);

    const float coef08 = shz_fmaf(mtrx->elem2D[1][2], mtrx->elem2D[2][3],
                                  -mtrx->elem2D[1][3] * mtrx->elem2D[2][2]);
    const float coef10 = shz_fmaf(mtrx->elem2D[1][1], mtrx->elem2D[2][3],
                                  -mtrx->elem2D[1][3] * mtrx->elem2D[2][1]);
    const float coef11 = shz_fmaf(mtrx->elem2D[1][1], mtrx->elem2D[2][2],
                                  -mtrx->elem2D[1][2] * mtrx->elem2D[2][1]);

    const float coef12 = shz_fmaf(mtrx->elem2D[0][2], mtrx->elem2D[3][3],
                                  -mtrx->elem2D[0][3] * mtrx->elem2D[3][2]);
    const float coef14 = shz_fmaf(mtrx->elem2D[0][1], mtrx->elem2D[3][3],
                                  -mtrx->elem2D[0][3] * mtrx->elem2D[3][1]);
    const float coef15 = shz_fmaf(mtrx->elem2D[0][1], mtrx->elem2D[3][2],
                                  -mtrx->elem2D[0][2] * mtrx->elem2D[3][1]);

    const float coef16 = shz_fmaf(mtrx->elem2D[0][2], mtrx->elem2D[2][3],
                                  -mtrx->elem2D[0][3] * mtrx->elem2D[2][2]);
    const float coef18 = shz_fmaf(mtrx->elem2D[0][1], mtrx->elem2D[2][3],
                                  -mtrx->elem2D[0][3] * mtrx->elem2D[2][1]);
    const float coef19 = shz_fmaf(mtrx->elem2D[0][1], mtrx->elem2D[2][2],
                                  -mtrx->elem2D[0][2] * mtrx->elem2D[2][1]);

    const float coef20 = shz_fmaf(mtrx->elem2D[0][2], mtrx->elem2D[1][3],
                                  -mtrx->elem2D[0][3] * mtrx->elem2D[1][2]);
    const float coef22 = shz_fmaf(mtrx->elem2D[0][1], mtrx->elem2D[1][3],
                                  -mtrx->elem2D[0][3] * mtrx->elem2D[1][1]);
    const float coef23 = shz_fmaf(mtrx->elem2D[0][1], mtrx->elem2D[1][2],
                                  -mtrx->elem2D[0][2] * mtrx->elem2D[1][1]);

    const shz_vec4_t fac0 = shz_vec4_init(coef00, coef00, coef02, coef03);
    const shz_vec4_t fac1 = shz_vec4_init(coef04, coef04, coef06, coef07);
    const shz_vec4_t fac2 = shz_vec4_init(coef08, coef08, coef10, coef11);
    const shz_vec4_t fac3 = shz_vec4_init(coef12, coef12, coef14, coef15);
    const shz_vec4_t fac4 = shz_vec4_init(coef16, coef16, coef18, coef19);
    const shz_vec4_t fac5 = shz_vec4_init(coef20, coef20, coef22, coef23);

    const shz_vec4_t vec0 =
        shz_vec4_init(mtrx->elem2D[0][1], mtrx->elem2D[0][0],
                      mtrx->elem2D[0][0], mtrx->elem2D[0][0]);
    const shz_vec4_t vec1 =
        shz_vec4_init(mtrx->elem2D[1][1], mtrx->elem2D[1][0],
                      mtrx->elem2D[1][0], mtrx->elem2D[1][0]);
    const shz_vec4_t vec2 =
        shz_vec4_init(mtrx->elem2D[2][1], mtrx->elem2D[2][0],
                      mtrx->elem2D[2][0], mtrx->elem2D[2][0]);
    const shz_vec4_t vec3 =
        shz_vec4_init(mtrx->elem2D[3][1], mtrx->elem2D[3][0],
                      mtrx->elem2D[3][0], mtrx->elem2D[3][0]);

    const shz_vec4_t inv0 = shz_vec4_add(
        shz_vec4_sub(shz_vec4_mul(vec1, fac0), shz_vec4_mul(vec2, fac1)),
        shz_vec4_mul(vec3, fac2));
    const shz_vec4_t inv1 = shz_vec4_add(
        shz_vec4_sub(shz_vec4_mul(vec0, fac0), shz_vec4_mul(vec2, fac3)),
        shz_vec4_mul(vec3, fac4));
    const shz_vec4_t inv2 = shz_vec4_add(
        shz_vec4_sub(shz_vec4_mul(vec0, fac1), shz_vec4_mul(vec1, fac3)),
        shz_vec4_mul(vec3, fac5));
    const shz_vec4_t inv3 = shz_vec4_add(
        shz_vec4_sub(shz_vec4_mul(vec0, fac2), shz_vec4_mul(vec1, fac4)),
        shz_vec4_mul(vec2, fac5));

    const shz_vec4_t signA = shz_vec4_init(+1, -1, +1, -1);
    const shz_vec4_t signB = shz_vec4_init(-1, +1, -1, +1);

    out->col[0] = shz_vec4_mul(inv0, signA);
    out->col[1] = shz_vec4_mul(inv1, signB);
    out->col[2] = shz_vec4_mul(inv2, signA);
    out->col[3] = shz_vec4_mul(inv3, signB);

    const float inv_det = shz_invf(
        shz_fmaf(mtrx->elem2D[0][0], out->elem2D[0][0],
                 shz_fmaf(mtrx->elem2D[1][0], out->elem2D[1][0],
                          shz_fmaf(mtrx->elem2D[2][0], out->elem2D[2][0],
                                   mtrx->elem2D[3][0] * out->elem2D[3][0]))));

    for (int c = 0; c < 4; c++) {
        out->col[c] = shz_vec4_scale(out->col[c], inv_det);
    }
}