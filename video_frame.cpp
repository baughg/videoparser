#include "video_frame.h"
#include <cstddef>

VideoFrame::VideoFrame(void): frame_length(0),frame_number(0)
{
}


VideoFrame::~VideoFrame(void)
{
}

void VideoFrame::SetFrameNumber(uint64 frame_num) {
	frame_number = frame_num;
}

uint64 VideoFrame::FrameNumber() {
	return frame_number;
}

void VideoFrame::Allocate(uint64 frame_size) {
	if(frame_size <= 0)
		return;

	rgb_frame.resize(frame_size*3);	
	y_frame.resize(frame_size);	
	frame_length = frame_size;
}



pixel* VideoFrame::rgbPointer() {
	pixel* ptr = nullptr;

	if(frame_length > 0)
		ptr = &rgb_frame.at(0);

	return ptr;
}




pixel* VideoFrame::yPointer() {
	pixel* ptr = nullptr;

	if(frame_length > 0)
		ptr = &y_frame.at(0);

	return ptr;
}
