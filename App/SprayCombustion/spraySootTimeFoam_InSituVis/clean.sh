#!/bin/bash
PROGRAM=spraySootTimeFoam

if [ -z $WM_DIR ]; then
    of240
fi

wclean libso sootMoment
wclean

if [ -e $PROGRAM ]; then
    rm $PROGRAM
fi
