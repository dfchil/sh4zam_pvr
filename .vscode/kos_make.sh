#!/usr/bin/env bash

TARGET=part_n+3_specular.elf

#set the KOS environtment variables
source /opt/toolchains/dc/kos/environ.sh
DCTRACE=1 DEBUG=1 BASEPATH="/pc" make clean
# DCPROF=1 SINGLEDEMO=7 
SHOWFRAMETIMES=1 DEBUG=1 OPTLEVEL=g  BASEPATH=/pc make $TARGET -j 10
exit