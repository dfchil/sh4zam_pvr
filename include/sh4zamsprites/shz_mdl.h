#ifndef shzmdl_H
#define shzmdl_H

#include <stdint.h>
#include <sh4zam/shz_sh4zam.h>

typedef enum : uint8_t {
  shzmdl_UNTEXTURED = 0,
  shzmdl_TEXTURE_COORDS = 1,
  shzmdl_VERTEX_NORMALS = 2,
  shzmdl_TEXTURED_VERTEX_NORMALS = 3,
  shzmdl_FACE_NORMALS = 4,
  shzmdl_TEXTURED_FACE_NORMALS = 5,
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
  shz_vec3_t center_normal;
  void* next_fan;
} shz_mdl_fan_start_t;

typedef struct __attribute__((packed)) {
  struct {
    uint8_t major; // marks incompatible changes
    uint8_t minor; // marks additional features, backward compatible
    uint8_t patch; // bug fixes, backward compatible
    uint8_t flags; // reserved for future use
  } version;
  struct {
    uint32_t tri_faces;
    uint32_t quad_faces;
    uint32_t fans;
    uint32_t strips;
  } offset;  // in units of 32 bytes
  struct {
    uint32_t tri_faces;
    uint32_t quad_faces;
  } num;
  shz_mdl_type_e type;
} shzmdl_hdr_t;

#endif // shzmdl_H
