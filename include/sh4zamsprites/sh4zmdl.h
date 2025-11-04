#ifndef SH4ZMDL_H
#define SH4ZMDL_H

#include <stdint.h>
#include <sh4zam/shz_sh4zam.h>

typedef enum : uint8_t {
  SHZ_MDL_TRI_FACES = 1,
  SHZ_MDL_QUAD_FACES = 2,
  SHZ_MDL_QUADS_N_TRIS = 3,
  SHZ_MDL_TRI_STRIPS = 4,
  SHZ_MDL_QUAD_STRIPS = 5,
  SHZ_MDL_TRI_N_QUAD_STRIPS = 6,
} sh4zmdl_type_e;


typedef struct __attribute__((packed)) sh4zmdl_tri_face_t {
  shz_vec3_t normal;
  shz_vec3_t v1;
  shz_vec3_t v2;
  shz_vec3_t v3;
} sh4zmdl_tri_face_t;

typedef struct __attribute__((packed)) {
  shz_vec3_t normal;
  shz_vec3_t v1;
  shz_vec3_t v2;
  shz_vec3_t v3;
  shz_vec3_t v4;
} sh4zmdl_quad_face_t;

typedef struct __attribute__((packed)) {
  uint32_t num_quads;
} sh4zmdl_quad_strip_t;


typedef struct __attribute__((packed)) {
  union {
    struct {
      uint16_t tri_faces;
      uint16_t quad_faces;
    };
    struct {
      uint16_t tri_strips;
      uint16_t quad_strips;
    };
  } num;
  sh4zmdl_type_e type;
  uint8_t padding[27];
} sh4zmdl_hdr_t;

#endif // SH4ZMDL_H
