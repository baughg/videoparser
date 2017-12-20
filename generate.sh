#!/bin/bash
rm -rf dump
rm -rf dumpt
rm -rf face
rm -rf facesel
rm hist.dat
rm *.bmp
rm *.png
rm frame_rank.dat
rm frame_rank_thumb.dat
rm face_detection.sh
rm face_resize.sh
rm face_album.sh
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
mkdir facesel
vdecoder facepull $1
vdecoder facesel $1
chmod +x face_resize.sh
./face_resize.sh
vdecoder facealbum $1
chmod +x face_album.sh
./face_album.sh
