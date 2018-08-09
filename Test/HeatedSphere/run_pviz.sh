#!/bin/bash
#PBS -N ParaRender
#PBS -q uv-test
#PBS -o stdout.log
#PBS -e stderr.log
#PBS -l select=1:ncpus=4:mpiprocs=4

cd ${PBS_O_WORKDIR}
PROGRAM=AnotherRendering
MPIEXEC=mpiexec_mpt
export KMP_AFFINITY=disabled
export OMP_NUM_THREADS=4
export MPI_SHEPHERD=true

NPROCS=4
NREGIONS=64 # Number of regions
DATA=~/Work/Data/HeatedSphere/sphere_muto_160615/heatSphere_euler_50050.fv
WIDTH=512
HEIGHT=512

$MPIEXEC -np $NPROCS omplace -nt ${OMP_NUM_THREADS} ./$PROGRAM $DATA -regions $NREGIONS -width $WIDTH -height $HEIGHT
