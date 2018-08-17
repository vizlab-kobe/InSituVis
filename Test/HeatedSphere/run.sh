#!/bin/bash
PROGRAM=${PWD##*/}
MPIEXEC=mpiexec

NPROCS=4
NREGIONS=64 # Number of regions
MAPPING=0 # 0: Isosurface, 1: Slice Plane, 2: External Face (Not supported)
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
WIDTH=512
HEIGHT=512

$MPIEXEC --oversubscribe -np $NPROCS ./$PROGRAM $DATA -regions $NREGIONS -mapping $MAPPING -width $WIDTH -height $HEIGHT
