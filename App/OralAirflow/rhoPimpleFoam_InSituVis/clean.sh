#!/bin/sh
PROGRAM=rhoPimpleFoam_InSituVis

if [ -z $WM_DIR ]; then
    source ../etc/bashrc
fi

wclean

if [ -e $PROGRAM ]; then
    rm $PROGRAM
fi
