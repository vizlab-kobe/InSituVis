#!/bin/bash
PROGRAM=${PWD##*/}
MPIEXEC=mpiexec

NPROCS=4 # Number of processes
NREGIONS=64 # Number of regions
SAMPLING_METHOD=1 # 0:uniform, 1:metropolis, 2:rejection, 3:layered, 4:point
NREPEATS=1 # Number of repetitions
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
WIDTH=512
HEIGHT=512

if [ $# = 1 ] && [ $1 = "multi" ]; then
    for REPEATS in 1
    do
	for NPROCS in 1 2 4
	do
	    NREGIONS=$[256/$NPROCS]
	    $MPIEXEC --oversubscribe -np $NPROCS ./$PROGRAM $DATA -sampling_method $SAMPLING_METHOD -regions $NREGIONS -repeats $NREPEATS -width $WIDTH -height $HEIGHT
	    echo ""
	done
    done
else
	    $MPIEXEC --oversubscribe -np $NPROCS ./$PROGRAM $DATA -sampling_method $SAMPLING_METHOD -regions $NREGIONS -repeats $NREPEATS -width $WIDTH -height $HEIGHT
fi
