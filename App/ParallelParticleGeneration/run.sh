#!/bin/bash
PROGRAM=${PWD##*/}
#MPIEXEC=mpiexec
MPIEXEC=mpirun

NPROCS=2
NTHREADS=1
SAMPLING_METHOD=1 # [0:uniform, 1:metropolis, 2:rejection, 3:layered]
NREGIONS=64 # Number of regions
NREPEATS=1
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
WIDTH=512
HEIGHT=512

#$MPIEXEC -np $NPROCS -nt $NTHREADS ./$PROGRAM $DATA -sampling_method $SAMPLING_METHOD -regions $NREGIONS -repeats $NREPEATS -width $WIDTH -height $HEIGHT
$MPIEXEC --oversubscribe -np $NPROCS ./$PROGRAM $DATA -sampling_method $SAMPLING_METHOD -regions $NREGIONS -repeats $NREPEATS -width $WIDTH -height $HEIGHT
