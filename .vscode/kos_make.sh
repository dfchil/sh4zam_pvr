#!/usr/bin/env bash

TARGET=part_4_pvr_sprites
# TARGET=part_n+1_specular_triangle_faces_stl.elf

#set the KOS environtment variables
source /opt/toolchains/dc/kos/environ.sh
cd examples/${TARGET}
make clean
# DCPROF=1 SINGLEDEMO=7 
ENJ_SHOWFRAMETIMES=1 ENJ_DEBUG=1 ENJ_OPTLEVEL=3  BASEPATH=/pc make  -j 10
exit
