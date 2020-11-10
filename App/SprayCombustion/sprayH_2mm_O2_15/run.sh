#!/bin/sh
PROGRAM=spraySootTimeFoam
NTHREADS=6

if [ -z $WM_DIR ]; then
    of240
fi

OMP_NUM_THREADS=$NTHREADS ../$PROGRAM/$PROGRAM
