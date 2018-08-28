#!/bin/bash
PROGRAM=${PWD##*/}
MPIEXEC=mpiexec

NPROCS=4 # Number of processes
NREGIONS=64 # Number of regions
MAPPING=0 # 0: Isosurface, 1: Slice Plane, 2: External Face (Not supported)
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
WIDTH=512
HEIGHT=512

if [ $# = 1 ] && [ $1 = "multi" ]; then
    for NPROCS in 1 2 4
    do
	NREGIONS=$[256/$NPROCS]
	$MPIEXEC --oversubscribe -np $NPROCS ./$PROGRAM $DATA -regions $NREGIONS -mapping $MAPPING -width $WIDTH -height $HEIGHT
	echo ""
    done
else
    $MPIEXEC --oversubscribe -np $NPROCS ./$PROGRAM $DATA -regions $NREGIONS -mapping $MAPPING -width $WIDTH -height $HEIGHT
fi
