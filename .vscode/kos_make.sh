#!/usr/bin/env bash

TARGET=part_n_specular_quads_n_fans.elf
# TARGET=part_n+1_specular_triangle_faces_stl.elf

#set the KOS environtment variables
source /opt/toolchains/dc/kos/environ.sh
make clean
# DCPROF=1 SINGLEDEMO=7 
SHOWFRAMETIMES=1 DEBUG=1 OPTLEVEL=3  BASEPATH=/pc make $TARGET -j 10
exit