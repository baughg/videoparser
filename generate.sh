#!/bin/bash
rm -rf dump
rm hist.dat
rm *.bmp
rm frame_rank.dat
echo running $1
vdecoder hist $1
vdecoder rank $1
mkdir dump
vdecoder thumb $1