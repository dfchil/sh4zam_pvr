#ifndef shzmdl_H
#define shzmdl_H

#include <stdint.h>
#include <sh4zam/shz_sh4zam.h>

typedef enum : uint8_t {
  shzmdl_TRI_FACES = 1,
  shzmdl_QUAD_FACES = 2,
  shzmdl_QUADS_N_TRIS = 3,
  shzmdl_TRI_STRIPS = 4,
  shzmdl_QUAD_STRIPS = 5,
  shzmdl_TRI_N_QUAD_STRIPS = 6,
} shzmdl_type_e;


typedef struct __attribute__((packed)) shzmdl_tri_face_t {
  shz_vec3_t normal;
  shz_vec3_t v1;
  shz_vec3_t v2;
  shz_vec3_t v3;
} shzmdl_tri_face_t;

typedef struct __attribute__((packed)) {
  shz_vec3_t normal;
  shz_vec3_t v1;
  shz_vec3_t v2;
  shz_vec3_t v3;
  shz_vec3_t v4;
} shzmdl_quad_face_t;


typedef struct __attribute__((packed)) {
  shz_vec3_t vert;
  shz_vec3_t normal;
} shzmdl_vert_normal_t;

typedef struct __attribute__((packed)) {
  uint32_t num_tris;
  shzmdl_vert_normal_t center;
  shzmdl_vert_normal_t verts[];
} shzmdl_fan_t;

typedef struct __attribute__((packed)) {
  uint32_t num_tris;
  shzmdl_vert_normal_t verts[];
} shzmdl_tri_strip_hdr_t;

typedef struct __attribute__((packed)) {
  uint32_t num_quads;
} shzmdl_quad_strip_hdr_t;

typedef struct __attribute__((packed)) {
  shz_vec3_t v1;
  shz_vec3_t v2;
  shz_vec3_t normal;
} shzmdl_quad_normal_t;


typedef struct __attribute__((packed)) {
  uint32_t num_quads;
} shzmdl_quad_strip_t;


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
  shzmdl_type_e type;
  uint8_t padding[27];
} shzmdl_hdr_t;

#endif // shzmdl_H
