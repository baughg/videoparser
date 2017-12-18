#pragma once
#include<string>
#include<vector>
#include<stdint.h>

//#include "FrameBlock.h"
#include "video_frame.h"
#include "rgb_histogram.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
#include <libavutil/avstring.h>
}



struct VideoStreamState {
	AVFormatContext* pFormatCtx;
	//int i;
	int	videoStream;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVFrame* pFrame; 
	AVFrame* pFrameRGB;
	AVPacket packet;
	int frameFinished;
	int numBytes;
	uint8_t* buffer;
	AVDictionary* optionsDict;
	struct SwsContext* sws_ctx;
	uint64 *min_frame;
	uint64 *max_frame;
	uint64 *packet_count;
	std::vector<uint64> frame_start;
};



class FrameServer
{
public:
	FrameServer(void);
	~FrameServer(void);
	int Parse(std::string _video_file, int mode = 0, uint32_t select_frames = 150);
	
	VideoStreamState video_stream_state;
	static void SaveFrame(AVFrame *pFrameY, AVFrame *pFrame, size_t width, size_t height, long long iFrame,FramePointer &frame_pointer,bool run_ip=false,FrameServer* server=NULL);
	static void WriteBitmap(std::string filename,int _width,int _height,int planes,unsigned char* dataPtr);	
	static void ReadBitmap(std::string filename,int &width, int &height, int &planes, std::vector<uint8_t> &bitmap_image);
	bool quit;
	size_t Width();
	size_t Height();
	pixel* FirstFrame(uint64 &frame_num);
	uint64 Length();
	
	uint64 CurrentFrame();
	void ReadStream(std::vector<uint64> &frame_start, int mode = 0);
	RGBHistogram & get_global_histogram();
private:
	void rgb2bgr(pixel* rgb_ptr, size_t width,size_t height);

	void RGBToGray(pixel* rgb_ptr, pixel* gray_ptr,size_t width,size_t height);
	
	size_t width;
	size_t height;
	size_t pel_count;
	
	void InitializeBuffer();	
	uint64 frame_count;
	std::string video_file;	
	AVFormatContext* pFormatCtx;
	//int i;
	int	videoStream;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVFrame* pFrame; 
	AVFrame* pFrameRGB;
	AVPacket packet;
	int frameFinished;
	int numBytes;
	uint8_t* buffer;
	AVDictionary* optionsDict;
	struct SwsContext* sws_ctx;
	std::vector<uint64> frame_start;		
	RGBHistogram frame_rgb_hist_;
	RGBHistogram global_rgb_hist_;
};

