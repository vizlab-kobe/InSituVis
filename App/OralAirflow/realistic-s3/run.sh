#!/bin/sh
PROGRAM=rhoPimpleFoam_InSituVis

mpirun -n 8 ../$PROGRAM/$PROGRAM -parallel
