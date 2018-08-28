#!/bin/bash
#PBS -N GatherParticle
#PBS -q uv-test
#PBS -o stdout.log
#PBS -e stderr.log
#PBS -l select=1:ncpus=4:mpiprocs=4

cd ${PBS_O_WORKDIR}
PROGRAM=GatherParticle
MPIEXEC=mpiexec_mpt
export KMP_AFFINITY=disabled
export OMP_NUM_THREADS=4
export MPI_SHEPHERD=true

NPROCS=4
SAMPLING_METHOD=1 # [0:uniform, 1:metropolis, 2:rejection, 3:layered]
NREGIONS=64 # Number of regions
NREPEATS=1
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
WIDTH=512
HEIGHT=512

$MPIEXEC -np $NPROCS omplace -nt ${OMP_NUM_THREADS} ./$PROGRAM $DATA -sampling_method $SAMPLING_METHOD -regions $NREGIONS -repeats $NREPEATS -width $WIDTH -height $HEIGHT
