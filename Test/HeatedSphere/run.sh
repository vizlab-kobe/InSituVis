#!/bin/bash
PROGRAM=${PWD##*/}
MPIEXEC=mpiexec

NPROCS=4
NREGIONS=64 # Number of regions
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
WIDTH=512
HEIGHT=512

$MPIEXEC --oversubscribe -np $NPROCS ./$PROGRAM $DATA -regions $NREGIONS -width $WIDTH -height $HEIGHT
