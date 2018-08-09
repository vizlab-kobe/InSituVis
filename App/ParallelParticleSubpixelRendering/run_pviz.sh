#!/bin/bash
#PBS -N SubpixelRender
#PBS -q uv-large
#PBS -o stdout.log
#PBS -e stderr.log
#PBS -l select=1:ncpus=64:mpiprocs=64

cd ${PBS_O_WORKDIR}
PROGRAM=SubpixelRendering
MPIEXEC=mpiexec_mpt

#NPROCS=64
SAMPLING_METHOD=1 # [0:uniform, 1:metropolis, 2:rejection, 3:layered]
NREGIONS=4 # Number of regions
#NREPEATS=25
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
#WIDTH=512
#HEIGHT=512

for SIZE in 4096
do
    for NREPEATS in 1 
    do
	for NPROCS in 2 4 8 16 32 64
	do
	    $MPIEXEC -np $NPROCS dplace ./$PROGRAM $DATA -sampling_method $SAMPLING_METHOD -regions $NREGIONS -repeats $NREPEATS -width $SIZE -height $SIZE
	done
    done
done
