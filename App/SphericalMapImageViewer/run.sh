#!/bin/sh
PROGRAM=${PWD##*/}
#FILENAME=~/Work/Data/4DStreatViewData/images/filename0000_0000.bmp
FILENAME=test.bmp
./$PROGRAM -f $FILENAME
