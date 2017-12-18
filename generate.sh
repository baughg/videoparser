#!/bin/bash
rm -rf dump
rm -rf dumpt
rm -rf face
rm hist.dat
rm *.bmp
rm frame_rank.dat
rm frame_rank_thumb.dat
rm face_detection.sh
echo running $1
vdecoder hist $1
vdecoder rank $1
mkdir dump
mkdir dumpt
vdecoder thumb $1
vdecoder face $1
chmod +x face_detection.sh
./face_detection.sh
mkdir face
vdecoder facepull $1
