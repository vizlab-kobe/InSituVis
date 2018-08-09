#!/bin/bash
#PBS -N ParaRender
#PBS -q uv-large
#PBS -o stdout.log
#PBS -e stderr.log
#PBS -l select=1:ncpus=8:mpiprocs=8

cd ${PBS_O_WORKDIR}
PROGRAM=ParallelRendering
MPIEXEC=mpiexec_mpt

#NPROCS=8
SAMPLING_METHOD=1 # [0:uniform, 1:metropolis, 2:rejection, 3:layered]
NREGIONS=32 # Number of regions
#NREPEATS=25
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
#WIDTH=512
#HEIGHT=512

for NREPEATS in 1 25 100 225 400 
do
    for SIZE in 4096
    do
	for NPROCS in 8 4 2
	do
	    $MPIEXEC -np $NPROCS dplace ./$PROGRAM $DATA -sampling_method $SAMPLING_METHOD -regions $NREGIONS -repeats $NREPEATS -width $SIZE -height $SIZE
    done
done
