#!/bin/bash
PS_COUNT=`ps aux | grep spraySootTimeFoam | grep -v grep | wc -l`
if [ $PS_COUNT -gt 0 ]; then
    killall spraySootTimeFoam
fi

OUTPUT_DIR=Output
if [ -e $OUTPUT_DIR ]; then
    rm -rf $OUTPUT_DIR
fi

rm -rf 0.0*
rm -rf *e-*
