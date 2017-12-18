#!/bin/bash
rm -rf dump
rm hist.dat
rm *.bmp
rm frame_rank.dat
rm face_detection.sh
echo running $1
vdecoder hist $1
vdecoder rank $1
mkdir dump
vdecoder thumb $1
vdecoder face $1
chmod +x face_detection.sh
./face_detection.sh
rm -rf face
mkdir face
vdecoder facepull $1
