# :robot: Mesh IO

Trying to deal with mesh IO generically is even worse than image IO.

That being the case, the microcosm solution is to not even try to deal with it generically.  Only the most common configurations of two formats are currently supported using separate data structures whose sole purpose is to handle IO.

------

### FileOBJ

This represents a Wavefront OBJ mesh.  The full OBJ specification can technically represent separate objects, shading groups, and even things like curves.  The implementation here doesn’t bother with that—the supported syntax includes vertex positions `v`, optional normals `vn`, optional texture coordinates `vt`, and faces `f` (with any number of vertices, not just triangles).

The implementation will also keep track of material names in `usemtl` declarations.  However, no parsing of the associated Wavefront MTL format is performed.  So for example, the following cube OBJ will load and save just fine.

```
# Blender v2.82 (sub 7) OBJ File: ''
# www.blender.org
mtllib Cube.mtl
o Cube
v 1 1 -1
v 1 -1 -1
v 1 1 1
v 1 -1 1
v -1 1 -1
v -1 -1 -1
v -1 1 1
v -1 -1 1
vn 0 1 0
vn 0 0 1
vn -1 0 0
vn 0 -1 0
vn 1 0 0
vn 0 0 -1
vt 0.625 0.500
vt 0.875 0.500
vt 0.875 0.750
vt 0.625 0.750
vt 0.375 0.750
vt 0.625 1.000
vt 0.375 1.000
vt 0.375 0.000
vt 0.625 0.000
vt 0.625 0.250
vt 0.375 0.250
vt 0.125 0.500
vt 0.375 0.500
vt 0.125 0.750
usemtl Material
s off
f 1/1/1 5/2/1 7/3/1 3/4/1
f 4/5/2 3/4/2 7/6/2 8/7/2
f 8/8/3 7/9/3 5/10/3 6/11/3
f 6/12/4 2/13/4 4/5/4 8/14/4
f 2/13/5 1/1/5 3/4/5 4/5/5
f 6/11/6 5/10/6 1/1/6 2/13/6
```

Note that the `mtllib`, `o`, and `s` declarations will be ignored, but the material name from `usemtl Material` will be preserved.

------

### FilePLY

This represents a Stanford PLY mesh.  PLY is intended to be generic (perhaps to a fault) in its representation of vertex attributes.  The microcosm implementation assumes the vertex format uses the same convention as Blender output.  That is, vertex positions given by `x`, `y`, `z`, optional normals given by `nx`, `ny`, `nz`, optional texture coordinates given by `s`, `t`, and optional RGB colors given by `r`, `g`, `b`.   The equivalent file corresponding to the cube from the OBJ example looks like this.

```
ply
format ascii 1.0
comment Created by Blender 2.82 (sub 7) - www.blender.org, source file: ''
element vertex 24
property float x
property float y
property float z
property float nx
property float ny
property float nz
property float s
property float t
element face 6
property list uchar uint vertex_indices
end_header
...
```

All of the actual vertex data has been omitted for brevity. 
