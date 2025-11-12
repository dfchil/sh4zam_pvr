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

class Vec3f ():
    def __init__(self, x:float, y:float, z:float):
        self.x:float = x
        self.y:float = y
        self.z:float = z

    def __eq__(self, value: object) -> bool:
        if not isinstance(value, Vec3f):
            return False
        return self.x == value.x and self.y == value.y and self.z == value.z
    
    def length(self) -> float:
        return (self.x**2 + self.y**2 + self.z**2) ** 0.5

    def scaled(self, scalar:float) -> 'Vec3f':
        return Vec3f(
            self.x * scalar,
            self.y * scalar,
            self.z * scalar
        )

    def normalized(self) -> 'Vec3f':
        length = self.length()
        if length == 0:
            return Vec3f(0.0, 0.0, 0.0)
        return self.scaled(1.0/length)
    
    def minussed(self, other:'Vec3f') -> 'Vec3f':
        return Vec3f(
            self.x - other.x,
            self.y - other.y,
            self.z - other.z
        )
    
    def plussed(self, other:'Vec3f') -> 'Vec3f':
        return Vec3f(
            self.x + other.x,
            self.y + other.y,
            self.z + other.z
        )

    def crossed(self, other:'Vec3f') -> 'Vec3f':
        return Vec3f(
            self.y * other.z - self.z * other.y,
            self.z * other.x - self.x * other.z,
            self.x * other.y - self.y * other.x
        )

    def dot(self, other:'Vec3f') -> float:
        return self.x * other.x + self.y * other.y + self.z * other.z


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
    def __init__(self, v0:VertexIndex, v1:VertexIndex, v2:VertexIndex):
        self.v0 = v0
        self.v1 = v1
        self.v2 = v2

    def __eq__(self, value: object) -> bool:
        if not isinstance(value, TriangleIndex):
            return False
        return (self.v0 == value.v0 and
                self.v1 == value.v1 and
                self.v2 == value.v2)

    def add2stl(self, model, stlfile):
        normal = model.normal(self)
        stlfile.write(struct.pack("<3f", normal.x, normal.y, normal.z))
        for vi in [self.v0, self.v1, self.v2]:
            vertex = model.vertices[vi.vertex_index]
            stlfile.write(struct.pack("<3f", vertex.x, vertex.y, vertex.z))
        stlfile.write(struct.pack("<H", 0))

class QuadIndex ():
    def __init__(self, v0:typing.Optional[VertexIndex], v1:typing.Optional[VertexIndex], v2:typing.Optional[VertexIndex], v3:typing.Optional[VertexIndex]):
        self.v0 = v0
        self.v1 = v1
        self.v2 = v2
        self.v3 = v3
    
    def add2stl(self, model, stlfile):
        normal = model.normal(self)
        stlfile.write(struct.pack("<3f", normal.x, normal.y, normal.z))
        for vi in [self.v0, self.v1, self.v2]:
            vertex = model.vertices[vi.vertex_index]
            stlfile.write(struct.pack("<3f", vertex.x, vertex.y, vertex.z))
        stlfile.write(struct.pack("<H", 0))
        stlfile.write(struct.pack("<3f", normal.x, normal.y, normal.z))
        for vi in [self.v0, self.v2, self.v3]:
            vertex = model.vertices[vi.vertex_index]
            stlfile.write(struct.pack("<3f", vertex.x, vertex.y, vertex.z))
        stlfile.write(struct.pack("<H", 0))

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
        self.vertices:list[Vec3f] = []
        self.vertex_normals:list[Vec3f] = []
        self.tex_coords:list[TexCoord2f] = []
        self.triangles:list[TriangleIndex] = []
        self.quads:list[QuadIndex] = []
        self.triangle_fans:list[TriangleFan] = []
        self.vertex_strips:list[list[VertexIndex]] = []

    def normal(self, face: typing.Union[TriangleIndex, QuadIndex]) -> Vec3f:
        if isinstance(face, TriangleIndex):
            v0 = self.vertices[face.v0.vertex_index]
            v1 = self.vertices[face.v1.vertex_index]
            v2 = self.vertices[face.v2.vertex_index]
            return v1.minussed(v0).crossed(v2.minussed(v0)).normalized()
        else:
            v0 = self.vertices[face.v0.vertex_index]
            v1 = self.vertices[face.v1.vertex_index]
            v2 = self.vertices[face.v2.vertex_index]
            v3 = self.vertices[face.v3.vertex_index]

            return v0.minussed(v2).crossed(v1.minussed(v3)).normalized()

    def load_from_obj(self, filepath:str):
        with open(filepath, "r") as f:
            for line in f:
                parts = line.strip().split()
                if not parts:
                    continue
                if parts[0] == 'v':
                    x, y, z = map(float, parts[1:4])
                    self.vertices.append(Vec3f(x, y, z))
                elif parts[0] == 'vn':
                    x, y, z = map(float, parts[1:4])
                    self.vertex_normals.append(Vec3f(x, y, z))
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
            for vi in [tri.v0, tri.v1, tri.v2]:
                fan_center = vi.vertex_index
                if fan_center not in potential_fans:
                    potential_fans[fan_center] = []
                potential_fans[fan_center].append(tri_idx)
        for fan_center in potential_fans:
            if len(potential_fans[fan_center]) > 10:
                pairs:dict[int, tuple[int, int]] = {}
                for tri_idx in potential_fans[fan_center]:
                    tri = self.triangles[tri_idx]
                    pair = [tri.v0.vertex_index, tri.v1.vertex_index, tri.v2.vertex_index]
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

                if fanorder[0][1] == fanorder[-1][1]:
                    fanorder.pop()  # remove duplicate last triangle

                #todo: handle fans with gaps
                first_tri:TriangleIndex = self.triangles[fanorder[0][1]]
                center_vert:VertexIndex = [v for v in [first_tri.v0, first_tri.v1, first_tri.v2] if v.vertex_index == fan_center][0]
                fan = TriangleFan(center_vert)
                for vi, tri_idx in fanorder:
                    tri = self.triangles[tri_idx]
                    nxt_vert = [v for v in [tri.v0, tri.v1, tri.v2] if v.vertex_index == vi]
                    fan.add_vertex(nxt_vert[0])
                output_fans.append(fan)

                # remove these triangles from the modelModel(vertices=3241, normals=3242, texcoords=1588, triangles=0, quads=3120)
                for _, tri_idx in fanorder:
                    self.triangles[tri_idx] = None  # mark for removal

        # clean up triangles
        self.triangles = [tri for tri in self.triangles if tri is not None]
        self.triangle_fans = output_fans

        return output_fans

    def fan_shed_quads(self, fan_idx:int, cut_length_from_center:float, combine_fans:int=1):
        fan = self.triangle_fans[fan_idx]
        center_coord = self.vertices[fan.center.vertex_index]
        new_vertices:list[Vec3f] = []
        new_quads:list[QuadIndex] = [QuadIndex(None, None, None, None) for i in range(len(fan.vertices))]
        new_blades:list[VertexIndex] = []

        for i in range(len(fan.vertices)):
            vi = fan.vertices[i]
            vert_coord = self.vertices[vi.vertex_index]
            new_coord = center_coord.plussed(
                vert_coord.minussed(
                    center_coord).scaled(cut_length_from_center)
                )
            new_vertidx = VertexIndex(len(self.vertices) + i, vi.normal_index, vi.texcoord_index)
            new_blades.append(new_vertidx)
            new_vertices.append(new_coord)

            new_quads[i].v0 = new_vertidx
            new_quads[i].v1 = vi
        
        if combine_fans > 1:  # move quad verts inward to center
            new_new_blades:list[VertexIndex] = []
            for i in range(0, len(new_vertices), combine_fans):
                new_new_blades.append(new_blades[i])
                a:Vec3f = new_vertices[i]
                end_idx = min(i + combine_fans +1, len(new_vertices))
                b:Vec3f = new_vertices[end_idx % len(new_vertices)]
                n = b.minussed(a).normalized()
                num_verts = min(combine_fans, len(new_vertices) - i)
                for idx in range(i + 1, end_idx): # skip first and last
                    if idx >= len(new_vertices):
                        break
                    vert_coord = new_vertices[idx]
                    # point on line
                    t = vert_coord.minussed(a).dot(n)
                    point_on_line = a.plussed(n.scaled(t))
                    new_vertices[idx] = point_on_line


            
            # merge fans
            new_blades = new_new_blades

            # remainder = (len(fan.vertices)+1) % len(new_new_blades)
            # print("Remainder:", remainder)
            # new_blades.extend(new_new_blades[-remainder:])

        for i in range(len(fan.vertices)):
            new_quads[(i-1) % len(fan.vertices)].v2 = new_quads[i].v1  # wrap around
            new_quads[(i-1) % len(fan.vertices)].v3 = new_quads[i].v0


        fan.vertices = new_blades
        self.vertices.extend(new_vertices)
        self.quads.extend(new_quads)
    
    def fan2triangles(self, fan_idx:int):
        new_triangles:list[TriangleIndex] = []
        fan = self.triangle_fans.pop(fan_idx)
        remaining_vertices = fan.vertices
        # make even
        while len(remaining_vertices) >= 3:
            if len(remaining_vertices) == 3:
                new_triangles.append(TriangleIndex(
                    remaining_vertices[0],
                    remaining_vertices[1],
                    remaining_vertices[2]
                ))
                remaining_vertices = []
                break
            if len(remaining_vertices) %2 != 0:
                new_triangles.append(TriangleIndex(
                    remaining_vertices[0],
                    remaining_vertices[1],
                    remaining_vertices[2]
                ))
                remaining_vertices.pop(1)
            next_remaining_vertices = []
            for f in range(0, len(remaining_vertices), 2):
                new_triangles.append(TriangleIndex(
                    remaining_vertices[f],
                    remaining_vertices[(f +1) % len(remaining_vertices)],
                    remaining_vertices[(f +2) % len(remaining_vertices)]
                ))
                next_remaining_vertices.append(remaining_vertices[f])
            remaining_vertices = next_remaining_vertices
        self.triangles.extend(new_triangles)


    def write_to_stl(self, filepath:str):
        num_triangles = 0
        with open(filepath, "wb") as f:
            f.seek(80+4)
            # f.write(struct.pack("<I", len(self.triangles) + len(self.quads) * 2 + sum(len(fan.vertices) for fan in self.triangle_fans)))

            for tri in self.triangles:
                tri.add2stl(self, f)
                num_triangles += 1

            for fan in self.triangle_fans:
                center_v = self.vertices[fan.center.vertex_index]
                prev_v:VertexIndex= fan.vertices[-1]
                for i in range(len(fan.vertices) +1):
                    next_v:VertexIndex = fan.vertices[i % len(fan.vertices)]
                    TriangleIndex(fan.center, prev_v, next_v).add2stl(self, f)
                    prev_v = next_v
                    num_triangles += 1

            for quad in self.quads:
                quad.add2stl(self, f)
                num_triangles += 2
            
            f.seek(80)
            f.write(struct.pack("<I", num_triangles))

    def write_to_obj(self, filepath:str):
        with open(filepath, "w") as f:
            for v in self.vertices:
                f.write(f"v {v.x} {v.y} {v.z}\n")
            for vn in self.vertex_normals:
                f.write(f"vn {vn.x} {vn.y} {vn.z}\n")
            for vt in self.tex_coords:
                f.write(f"vt {vt.u} {vt.v}\n")
            for tri in self.triangles:
                f.write(f"f {tri.v0.vertex_index +1}/{tri.v0.texcoord_index +1}/{tri.v0.normal_index +1} "
                             f"{tri.v1.vertex_index +1}/{tri.v1.texcoord_index +1}/{tri.v1.normal_index +1} "
                             f"{tri.v2.vertex_index +1}/{tri.v2.texcoord_index +1}/{tri.v2.normal_index +1}\n")
            for quad in self.quads:
                f.write(f"f {quad.v0.vertex_index +1}/{quad.v0.texcoord_index +1}/{quad.v0.normal_index +1} "
                             f"{quad.v1.vertex_index +1}/{quad.v1.texcoord_index +1}/{quad.v1.normal_index +1} "
                             f"{quad.v2.vertex_index +1}/{quad.v2.texcoord_index +1}/{quad.v2.normal_index +1} "
                             f"{quad.v3.vertex_index +1}/{quad.v3.texcoord_index +1}/{quad.v3.normal_index +1}\n")
            
            for fan in self.triangle_fans:
                center_v = self.vertices[fan.center.vertex_index]
                prev_v:VertexIndex= fan.vertices[-1]
                for i in range(len(fan.vertices) +1):
                    next_v:VertexIndex = fan.vertices[i % len(fan.vertices)]
                    tri = TriangleIndex(fan.center, prev_v, next_v)
                    f.write(f"f {tri.v0.vertex_index +1}/{tri.v0.texcoord_index +1}/{tri.v0.normal_index +1} "
                             f"{tri.v1.vertex_index +1}/{tri.v1.texcoord_index +1}/{tri.v1.normal_index +1} "
                             f"{tri.v2.vertex_index +1}/{tri.v2.texcoord_index +1}/{tri.v2.normal_index +1}\n")
                    prev_v = next_v


    def write_to_shzmdl(self, filepath:str):
        with open(filepath, "wb") as f:
            offset_triangles = 1 # follows right after model header
            triangle_data_size = (len(self.triangles) * 48)
            offset_quads = offset_triangles + (triangle_data_size >> 5) + (1 if triangle_data_size % 32 > 0 else 0)
            offset_fans = offset_quads + ((len(self.quads) * 64) >> 5)

            fan_verts = 0
            for fan in self.triangle_fans:
                f_verts = 32 + len(fan.vertices) * 3 * 2 * 4  # 32byte fan header + n * (vertex + normal) * 4bytes pr float
                if f_verts % 32 != 0:
                    f_verts += 32
                fan_verts += f_verts >> 5
            offset_strips = offset_fans + 1 + fan_verts # 32 byte fans header + fan data
            
            if len(self.triangles) == 0:
              offset_quads = offset_triangles
              offset_triangles = 0
            if len(self.quads) == 0:
              offset_fans = offset_quads
              offset_quads = 0
            if len(self.triangle_fans) == 0:
              offset_strips = offset_fans
              offset_fans = 0
            if len(self.vertex_strips) == 0:
              offset_strips = 0
            

            f.write(struct.pack("<4B", 0, 1, 0, 0))  # early version
            f.write(struct.pack("<I", offset_triangles))
            f.write(struct.pack("<I", offset_quads))
            f.write(struct.pack("<I", offset_fans))
            f.write(struct.pack("<I", offset_strips))

            f.write(struct.pack("<I", len(self.triangles)))
            f.write(struct.pack("<I", len(self.quads)))
            f.write(struct.pack("<B", 4))  # type: face normals no textures

            f.seek(offset_triangles << 5)
            for tri in self.triangles:
                normal = self.normal(tri)
                f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                for vi in [tri.v0, tri.v1, tri.v2]:
                    next_v = self.vertices[vi.vertex_index]
                    f.write(struct.pack("<3f", next_v.x, next_v.y, next_v.z))

            f.seek(offset_quads << 5)
            for quad in self.quads:
                v1, v2, v3, v4 = tuple(v for v in [quad.v0, quad.v1, quad.v2, quad.v3])
                normal = self.normal(quad)
                f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                for vi in [v1, v2, v3, v4]:
                    next_v = self.vertices[vi.vertex_index]
                    f.write(struct.pack("<3f", next_v.x, next_v.y, next_v.z))
                f.write(struct.pack("<I", 0))  # padding to 64 bytes per quad

            next_fan = offset_fans
            num_fans = len(self.triangle_fans)
            for fi in range(num_fans):
                f.seek(next_fan << 5)
                fan = self.triangle_fans[fi]
                f.write(struct.pack("<I", len(fan.vertices))) # num vertices in fan
                c_v = self.vertices[fan.center.vertex_index]
                f.write(struct.pack("<3f", c_v.x, c_v.y, c_v.z)) # fan center vert
                f.write(struct.pack("<3f", 0.0, 0.0, 0.0)) # dummy normal for center vert

                if (fi + 1) < num_fans:
                  next_fan += (32 + len(fan.vertices) * 3 * 2 * 4) >> 5 # 32byte fan header + n * (vertex + normal) * 4bytes pr float
                  f.write(struct.pack("<I", next_fan))
                else:
                  f.write(struct.pack("<I", 0))  # null for last fan

                prev_v:VertexIndex = fan.vertices[-1]
                for cur_v in fan.vertices:
                    cur_coord = self.vertices[cur_v.vertex_index]
                    f.write(struct.pack("<3f", cur_coord.x, cur_coord.y, cur_coord.z))
                    normal = self.normal(TriangleIndex(fan.center, prev_v, cur_v))
                    f.write(struct.pack("<3f", normal.x, normal.y, normal.z))
                    prev_v = cur_v

# source https://graphics.cs.utah.edu/courses/cs6620/fall2013/?prj=5
model = Model().load_from_obj(pwd + "/teapot2.obj")
# model.quads = []  # discard quads for STL export
# model.triangles = []



model.fan_triangles()

# model.fan_shed_quads(0, cut_length_from_center=0.6, combine_fans=1)
# model.fan_shed_quads(0, cut_length_from_center=0.5, combine_fans=5)

# model.fan_shed_quads(1, cut_length_from_center=0.6, combine_fans=1)
# model.fan_shed_quads(1, cut_length_from_center=0.5, combine_fans=5)

model.fan_shed_quads(0, cut_length_from_center=0.2, combine_fans=6)
model.fan_shed_quads(1, cut_length_from_center=0.2, combine_fans=6)

model.fan2triangles(1)
model.fan2triangles(0)

model.write_to_stl(pwd + "/teapot.stl")
model.write_to_shzmdl(pwd + "/teapot.shzmdl")
model.write_to_obj(pwd + "/teapot_out.obj")
print(model)