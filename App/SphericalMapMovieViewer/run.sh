#!/bin/sh
PROGRAM=${PWD##*/}
#FILENAME=~/Work/Data/4DStreatViewData/movies/movies0000.mp4
FILENAME=test.mp4
./$PROGRAM -f $FILENAME
