#!/bin/bash
PROGRAM=${PWD##*/}
MPIEXEC=mpiexec

NPROCS=4 # Number of processes
NREGIONS=64 # Number of regions
#NPROCS=1 # Number of processes
#NREGIONS=256 # Number of regions
SAMPLING_METHOD=1 # 0:uniform, 1:metropolis, 2:rejection, 3:layered, 4:point
SUBPIXELS=1 # Subpixel level
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
WIDTH=512
HEIGHT=512

$MPIEXEC --oversubscribe -np $NPROCS ./$PROGRAM $DATA -sampling_method $SAMPLING_METHOD -regions $NREGIONS -subpixels $SUBPIXELS -width $WIDTH -height $HEIGHT
