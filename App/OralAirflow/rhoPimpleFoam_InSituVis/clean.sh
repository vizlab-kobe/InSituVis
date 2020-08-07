#!/bin/sh
PROGRAM=rhoPimpleFoam_InSituVis

if [ -z $WM_DIR ]; then
    . ../etc/bashrc
fi

wclean

if [ -e $PROGRAM ]; then
    rm $PROGRAM
fi
