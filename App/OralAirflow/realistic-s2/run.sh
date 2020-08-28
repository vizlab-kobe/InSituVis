#!/bin/sh
PROGRAM=rhoPimpleFoam_InSituVis

if [ -z $WM_DIR ]; then
    . ../etc/bashrc
fi

mpirun -n 48 ../$PROGRAM/$PROGRAM -parallel

if type "ffmpeg" > /dev/null 2>&1; then
    ffmpeg -r 60 -start_number 00001 -i Output/output_%05d.png -vcodec libx264 -pix_fmt yuv420p -r 60 output.mp4
fi
