#!/bin/sh

# uses OBJXT to extract model vertices data and save it as C/C++ source code

./objxt -f examples/plyta.obj -of plyta.txt -c -v -cl Cube 217 185 97 -cl Cube0 217 185 97 -cl Cube1 50 50 50 -cl Cube2 100 100 100 -cl Cube3 100 100 100 -cl Cyl 100 100 100 -cl Cyl0 100 100 100 -cl Cyl1 100 100 100 -cl Cyl2 100 100 100 -cl Cyl3 100 100 100 -cl Cyl4 100 100 100 -cl Cyl5 100 100 100 -cl Cyl6 100 100 100 -cl Cyl7 100 100 100 -cl Cyl8 100 100 100 -cl Cyl9 100 100 100 

exit 0
