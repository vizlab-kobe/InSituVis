#!/bin/sh
PROGRAM=rhoPimpleFoam_InSituVis

if [ -z $WM_DIR ]; then
    . ../etc/bashrc
fi

mpirun -n 8 ../$PROGRAM/$PROGRAM -parallel
