#ifndef shzmdl_H
#define shzmdl_H

#include <stdint.h>
#include <sh4zam/shz_sh4zam.h>

typedef enum : uint8_t {
  shzmdl_UNTEXTURED = 0,
  shzmdl_TEXTURE_COORDS = 1,
  shzmdl_VERTEX_NORMALS = 2,
  shzmdl_FACE_NORMALS = 4,
} shz_mdl_type_e;

typedef struct __attribute__((packed)) shz_mdl_tri_face_t {
  shz_vec3_t normal;
  shz_vec3_t v1;
  shz_vec3_t v2;
  shz_vec3_t v3;
} shz_mdl_tri_face_t;

typedef struct __attribute__((packed)) {
  shz_vec3_t normal;
  shz_vec3_t v1;
  shz_vec3_t v2;
  shz_vec3_t v3;
  shz_vec3_t v4;
  char _padding[4];
} shz_mdl_quad_face_t;

typedef struct __attribute__((packed)) {
  shz_vec3_t vert;
  shz_vec3_t normal;
} shz_mdl_vert_normal_t;

typedef struct __attribute__((packed)) {
  uint32_t num_tris;
  shz_vec3_t center;
  void* next_fan;
  char _padding[12];
} shz_mdl_fan_face_normal_t;

typedef struct __attribute__((packed)) {
  uint32_t num_tris;
  shz_mdl_vert_normal_t center;
  void* next_fan;
} shz_mdl_fan_vert_normal_t;

typedef struct __attribute__((packed)) {
  struct {
    uint16_t tri_faces;
    uint16_t quad_faces;
    uint16_t tri_fans;
    uint16_t tri_strips;
  } offset;  // in units of 32 bytes
  struct {
    uint16_t tri_faces;
    uint16_t quad_faces;
  } num;

  shz_mdl_type_e type;
  uint8_t _padding[19];
} shzmdl_hdr_t;

#endif // shzmdl_H
