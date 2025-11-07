import os, os.path, struct
import typing


"""
model_brewser.py

This module is part of the Dreamcast Sprites of Sh4zam project.
It defines the data models and related functionality for the application.

Author: Daniel Fairchild
Date: 2023-10-04
"""


pwd = os.path.dirname(os.path.realpath(__file__))

class Coordinate3f ():
    def __init__(self, x:float, y:float, z:float):
        self.x:float = x
        self.y:float = y
        self.z:float = z

    def __eq__(self, value: object) -> bool:
        if not isinstance(value, Coordinate3f):
            return False
        return self.x == value.x and self.y == value.y and self.z == value.z

class TexCoord2f ():
    def __init__(self, u:float, v:float):
        self.u:float = u
        self.v:float = v  

class VertexIndex ():
    def __init__(self, v:int, n:int, t:int):
        self.vertex_index = v
        self.normal_index = n
        self.texcoord_index = t
    
    def __eq__(self, value: object) -> bool:
        if not isinstance(value, VertexIndex):
            return False
        return (self.vertex_index == value.vertex_index and
                self.normal_index == value.normal_index and
                self.texcoord_index == value.texcoord_index) 

class TriangleIndex ():
    def __init__(self, v1:VertexIndex, v2:VertexIndex, v3:VertexIndex):
        self.v1 = v1
        self.v2 = v2
        self.v3 = v3

class QuadIndex ():
    def __init__(self, v1:VertexIndex, v2:VertexIndex, v3:VertexIndex, v4:VertexIndex):
        self.v1 = v1
        self.v2 = v2
        self.v3 = v3
        self.v4 = v4

class TriangleFan ():
    def __init__(self, center:VertexIndex):
        self.center = center
        self.vertices:list[VertexIndex] = []

    def add_vertex(self, vertex:VertexIndex):
        self.vertices.append(vertex)
    
    def __str__(self) -> str:
        return "".join([f"{ v.vertex_index}->"  for v in self.vertices ])

class Model():
    def __init__(self):
        self.vertices:list[Coordinate3f] = []
        self.vertex_normals:list[Coordinate3f] = []
        self.tex_coords:list[TexCoord2f] = []
        self.triangles:list[TriangleIndex] = []
        self.quads:list[QuadIndex] = []
        self.triangle_fans:list[TriangleFan] = []

    def normal(self, face: typing.Union[TriangleIndex, QuadIndex]) -> Coordinate3f:
        v1 = self.vertices[face.v1.vertex_index]
        v2 = self.vertices[face.v2.vertex_index]
        v3 = self.vertices[face.v3.vertex_index] if isinstance(face, TriangleIndex) else self.vertices[face.v4.vertex_index]
        u = Coordinate3f(v2.x - v1.x, v2.y - v1.y, v2.z - v1.z)
        v = Coordinate3f(v3.x - v1.x, v3.y - v1.y, v3.z - v1.z)
        nx = u.y * v.z - u.z * v.y
        ny = u.z * v.x - u.x * v.z
        nz = u.x * v.y - u.y * v.x
        length = (nx**2 + ny**2 + nz**2) ** 0.5
        return Coordinate3f(nx / length, ny / length, nz / length)

    def load_from_obj(self, filepath:str):
        with open(filepath, "r") as f:
            for line in f:
                parts = line.strip().split()
                if not parts:
                    continue
                if parts[0] == 'v':
                    x, y, z = map(float, parts[1:4])
                    self.vertices.append(Coordinate3f(x, y, z))
                elif parts[0] == 'vn':
                    x, y, z = map(float, parts[1:4])
                    self.vertex_normals.append(Coordinate3f(x, y, z))
                elif parts[0] == 'vt':
                    u, v = map(float, parts[1:3])
                    self.tex_coords.append(TexCoord2f(u, v))
                elif parts[0] == 'f':
                    v_indices = []
                    for v in parts[1:]:
                        vals = v.split('/')
                        vi = int(vals[0]) - 1
                        ti = int(vals[1]) - 1 if len(vals) > 1 and vals[1] else -1
                        ni = int(vals[2]) - 1 if len(vals) > 2 and vals[2] else -1
                        v_indices.append(VertexIndex(vi, ni, ti))
                    if len(v_indices) == 3:
                        self.triangles.append(TriangleIndex(*tuple(v_indices)))
                    elif len(v_indices) == 4:
                        self.quads.append(QuadIndex(*tuple(v_indices)))
        return self
    
    def __str__(self) -> str:
        return f"""Model(
        vertices={len(self.vertices)}, 
        normals={len(self.vertex_normals)}, 
        texcoords={len(self.tex_coords)}, 
        triangles={len(self.triangles)}, 
        quads={len(self.quads)}, 
        fans={[len(f.vertices) for f in self.triangle_fans]})"""

    def fan_triangles(self):
        potential_fans:dict[int, list[int]] = {}
        output_fans:list[TriangleFan] = []
        for tri_idx in range(len(self.triangles)):
            tri = self.triangles[tri_idx]
            for vi in [tri.v1, tri.v2, tri.v3]:
                fan_center = vi.vertex_index
                if fan_center not in potential_fans:
                    potential_fans[fan_center] = []
                potential_fans[fan_center].append(tri_idx)
        for fan_center in potential_fans:
            if len(potential_fans[fan_center]) > 10:
                print("Fan at vertex", fan_center, "has", len(potential_fans[fan_center]), "triangles")
                pairs:dict[int, tuple[int, int]] = {}
                for tri_idx in potential_fans[fan_center]:
                    tri = self.triangles[tri_idx]
                    pair = [tri.v1.vertex_index, tri.v2.vertex_index, tri.v3.vertex_index]
                    pair.remove(fan_center)
                    pairs[pair[0]] = (pair[1], tri_idx)

                #start from lowest indexed vertex
                fanorder:list[tuple[int, int]] = [pairs[sorted(pairs.keys())[0]]]
                while len(pairs) > 0:
                    nxt:typing.Optional[tuple[int, int]] = pairs.pop(fanorder[-1][0], None)
                    if nxt is not None:
                        fanorder.append(nxt)
                    else:
                        break
                #todo: handle fans with gaps
                first_tri = self.triangles[fanorder[0][1]]
                center_verts = [v for v in [first_tri.v1, first_tri.v2, first_tri.v3] if v.vertex_index == fan_center]
                fan = TriangleFan(center_verts[0])
                for vi, tri_idx in fanorder:
                    tri = self.triangles[tri_idx]
                    nxt_vert = [v for v in [tri.v1, tri.v2, tri.v3] if v.vertex_index == vi]
                    fan.add_vertex(nxt_vert[0])
                output_fans.append(fan)
                print("  Fan:", fan)

                # remove these triangles from the modelModel(vertices=3241, normals=3242, texcoords=1588, triangles=0, quads=3120)
                for _, tri_idx in fanorder:
                    self.triangles[tri_idx] = None  # mark for removal

        # clean up triangles
        self.triangles = [tri for tri in self.triangles if tri is not None]
        self.triangle_fans = output_fans

        return output_fans
            

    def write_to_stl(self, filepath:str):
        with open(filepath, "wb") as f:
            f.seek(80)
            f.write(struct.pack("<I", len(self.triangles) + len(self.quads) * 2 + sum(len(fan.vertices) - 1 for fan in self.triangle_fans)))
            for tri in self.triangles:
                normal = self.normal(tri)
                f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                for vi in [tri.v1, tri.v2, tri.v3]:
                    vertex = self.vertices[vi.vertex_index]
                    f.write(struct.pack("<3f", vertex.x, vertex.y, vertex.z))
                f.write(struct.pack("<H", 0))
            for fan in self.triangle_fans:
                center_v = self.vertices[fan.center.vertex_index]
                cur_v:typing.Optional[VertexIndex] = None
                for next_v in fan.vertices:
                    if cur_v is not None:
                        normal = self.normal(TriangleIndex(fan.center, cur_v, next_v))
                        f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                        f.write(struct.pack("<3f", center_v.x, center_v.y, center_v.z))
                        cur_coord = self.vertices[cur_v.vertex_index]
                        f.write(struct.pack("<3f", cur_coord.x, cur_coord.y, cur_coord.z))
                        next_coord = self.vertices[next_v.vertex_index]
                        f.write(struct.pack("<3f", next_coord.x, next_coord.y, next_coord.z))
                        f.write(struct.pack("<H", 0))
                        cur_v = next_v
                    else:
                        cur_v = next_v

            for quad in self.quads:
                v1, v2, v3, v4 = quad.v1, quad.v2, quad.v3, quad.v4
                normal = self.normal(quad)
                f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                for vi in [v1, v2, v3]:
                    vertex = self.vertices[vi.vertex_index]
                    f.write(struct.pack("<3f", vertex.x, vertex.y, vertex.z))
                f.write(struct.pack("<H", 0))
                f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                for vi in [v1, v3, v4]:
                    vertex = self.vertices[vi.vertex_index]
                    f.write(struct.pack("<3f", vertex.x, vertex.y, vertex.z))
                f.write(struct.pack("<H", 0))

    def write_to_shz_mdl(self, filepath:str):
        with open(filepath, "wb") as f:

            offset_triangles = 1 + (1 if len(self.triangles) > 0 else 0) # follows right after model header
            offset_quads = offset_triangles + ((len(self.triangles) * 48) >> 5) + (1 if len(self.triangles) * 48 % 32 > 0 else 0)
            offset_fans = offset_quads + 1 + ((len(self.quads) * 64) >> 5)

            fan_verts = 0
            for fan in self.triangle_fans:
                f_verts = 32 + len(fan.vertices) * 3 * 2 * 4  # 32byte fan header + n * (vertex + normal) * 4bytes pr float
                if f_verts % 32 != 0:
                    f_verts += 32
                fan_verts += f_verts >> 5
            offset_strips = offset_fans + 1 + fan_verts # 32 byte fans header + fan data

            f.write(struct.pack("<H", offset_triangles))
            f.write(struct.pack("<H", offset_quads))
            f.write(struct.pack("<H", offset_fans))
            f.write(struct.pack("<H", offset_strips))

            f.write(struct.pack("<H", len(self.triangles)))
            f.write(struct.pack("<H", len(self.quads)))
            f.write(struct.pack("<B", 4))  # type: face normals no textures

            f.seek(offset_triangles << 5)
            for tri in self.triangles:
                normal = self.normal(tri)
                f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                for vi in [tri.v1, tri.v2, tri.v3]:
                    next_v = self.vertices[vi.vertex_index]
                    f.write(struct.pack("<3f", next_v.x, next_v.y, next_v.z))
            f.seek(offset_quads << 5)
            for quad in self.quads:
                v1, v2, v3, v4 = tuple(v for v in [quad.v1, quad.v2, quad.v3, quad.v4])
                normal = self.normal(quad)
                f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                for vi in [v1, v2, v3, v4]:
                    next_v = self.vertices[vi.vertex_index]
                    f.write(struct.pack("<3f", next_v.x, next_v.y, next_v.z))
            f.seek(offset_fans << 5)
            for fan in self.triangle_fans:
                c_v = self.vertices[fan.center.vertex_index]
                f.write(struct.pack("<3f", c_v.x, c_v.y, c_v.z))

                cur_v:typing.Optional[VertexIndex] = None
                for next_v in fan.vertices:
                    if cur_v is not None:
                        normal = self.normal(TriangleIndex(fan.center, cur_v, next_v))

                        cur_coord = self.vertices[cur_v.vertex_index]
                        f.write(struct.pack("<3f", cur_coord.x, cur_coord.y, cur_coord.z))
                        f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                        cur_v = next_v
                    else:
                        cur_v = next_v

                cur_coord = self.vertices[cur_v.vertex_index]
                f.write(struct.pack("<3f", cur_coord.x, cur_coord.y, cur_coord.z))
                f.write(struct.pack("<3f", 0.0, 0.0, 0.0)) # dummy normal for last vertex

# source https://graphics.cs.utah.edu/courses/cs6620/fall2013/?prj=5
model = Model().load_from_obj(pwd + "/teapot2.obj")
# model.quads = []  # discard quads for STL export
# model.triangles = []
model.fan_triangles()
model.write_to_stl(pwd + "/teapot.stl")
model.write_to_shz_mdl(pwd + "/teapot.shzmdl")
print(model)