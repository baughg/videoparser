#pragma once
//#include "videoconfig.h"
#include<vector>

typedef unsigned long long uint64;
typedef unsigned char pixel; 
typedef float p_float; 
typedef int label;
typedef std::vector<label> label_list;
typedef std::vector<pixel> pixel_list;
typedef std::vector<p_float> p_float_list;

class VideoFrame;

struct FramePointer {
	pixel* rgb_ptr;	
	pixel* y_ptr;	
};

typedef struct bgra
{
	pixel r;
	pixel g;
	pixel b;
	pixel a;
}bgra;

class VideoFrame
{
public:
	VideoFrame(void);
	~VideoFrame(void);
	void Allocate(uint64 frame_size);
	pixel* rgbPointer();
	
	pixel* yPointer();	
	void SetFrameNumber(uint64 frame_num);
	uint64 FrameNumber();	
private:
	pixel_list rgb_frame;	
	pixel_list y_frame;	
	uint64 frame_length;
	uint64 frame_number;	
};

