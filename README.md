# objxt
(A really simple) Wavefront .OBJ data extractor written in C++

## Using the program
When you compile and run OBJXT, the first thing you'll see is:
```
Usage:
./objxt <path to .obj> <flags>
Possible flags:
	-f = file path specifier
	-of = output file name
	-c = output C/C++ arrays of vertices
	-py = output Python array of vertices
	-v = dump vertices
	-n = dump normals
	-vt = dump vertex texture coordinates
	-cl = set color for selected object
```

OBJXT currently supports models consisting of triangles, it can parse all vertex definitions(lines starting with `v` in `.obj` files), vertex textures(`vt`), vertex normals `vn` and dump them as C++ or Python source code, or, a more "portable" binary file, which needs some careful parsing.

OBJXT can be configured to dump only certain data from parsed OBJ file, depending on this, the structure of output binary(and source code as well) file will differ.
All data has type `float`, which is typically `4 bytes`, so, each chunk of data (separated by `|` in table) in the table below can be parsed by reading next 4 bytes from the file 
```
   enabled by -v    enabled by -n      enabled by -vt    enabled by -cl
.-------------------------------------------------------------------------.
| v.x | v.y | v.z | vn.x | vn.y | vn.z | vt.x | vt.y | Red | Green | Blue |
'-------------------------------------------------------------------------'
```

If, for example, user has enabled only `-v` and set colors for figures in OBJ file, then his output binary will have order:
```
.--------------------------------------.
| v.x | v.y | v.z | Red | Green | Blue |
'--------------------------------------'
```
There's also a consideration to have: contrary to vertex data(coordinates, textures, normals), OBJXT doesn't pad color values, if some object doesn't have them specified, this will lead to more sophisticated binary file structure. Therefore, it's recommended to color ALL possible objects(defined by `o` in OBJ) from the extracted file. OBJXT logs all extracted object names, so you can figure what is left to color.

Coloring itself is done via `-cl` flag, which expects object name as first argument, followed by red, green and blue integer values:
```
-cl MyObject 255 255 0
```

Color value will be unused if `MyObject` was not defined in parsed OBJ file.
