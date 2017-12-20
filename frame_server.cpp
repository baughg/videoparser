#include "frame_server.h"
#include <fstream>
#include <stdio.h>
#include <cstddef>



FrameServer::FrameServer(void): frame_count(0),width(0),height(0),pel_count(0)	
{
	InitializeBuffer();
}


size_t FrameServer::Width() {
	return width;
}

size_t FrameServer::Height(){
	return height;
}

RGBHistogram & FrameServer::get_global_histogram()
{
	return global_rgb_hist_;
}

FrameServer::~FrameServer(void)
{
	
	quit = true;
	if (buffer == nullptr)
		return;

	if(buffer != nullptr)
		av_free(buffer);

	//printf("free buffer\n");
	if(pFrameRGB != nullptr)
		av_free(pFrameRGB);

	//printf("free pFrameRGB\n");
	// Free the YUV frame
	if(pFrame != nullptr)
		av_free(pFrame);

	//printf("free pFrame\n");
	// Close the codec
	if(pCodecCtx != nullptr)
		avcodec_close(pCodecCtx);

	//printf("free pCodecCtx\n");
	// Close the video file
	if(pFormatCtx != nullptr)
	avformat_close_input(&pFormatCtx);
	//printf("free pFormatCtx\n");
}

void FrameServer::InitializeBuffer() {
	pFormatCtx = nullptr;	
	pCodecCtx = nullptr;
	pCodec = nullptr;
	pFrame = nullptr; 
	pFrameRGB = nullptr;		
	buffer = nullptr;
	optionsDict = nullptr;
	sws_ctx = nullptr;	
}

int FrameServer::Parse(std::string _video_file, int mode, uint32_t select_frames) {
	video_file = _video_file;

	/*AVFormatContext *pFormatCtx = NULL;
	int             i, videoStream;
	AVCodecContext  *pCodecCtx = NULL;
	AVCodec         *pCodec = NULL;
	AVFrame         *pFrame = NULL; 
	AVFrame         *pFrameRGB = NULL;
	AVPacket        packet;
	int             frameFinished;
	int             numBytes;
	uint8_t         *buffer = NULL;

	AVDictionary    *optionsDict = NULL;
	struct SwsContext      *sws_ctx = NULL;*/

	// Register all formats and codecs
	av_register_all();

	// Open video file
	if(avformat_open_input(&pFormatCtx, video_file.c_str(), NULL, NULL)!=0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, video_file.c_str(), 0);
	int frame_data_size = 0;
	// Find the first video stream
	videoStream=-1;
	for(size_t i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			videoStream=i;
			break;
		}
		if(videoStream==-1)
			return -1; // Didn't find a video stream

		// Get a pointer to the codec context for the video stream
		pCodecCtx=pFormatCtx->streams[videoStream]->codec;

		// Find the decoder for the video stream
		pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
		if(pCodec==NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return -1; // Codec not found
		}
		// Open codec
		if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
			return -1; // Could not open codec

		// Allocate video frame
		pFrame=avcodec_alloc_frame();

		// Allocate an AVFrame structure
		pFrameRGB=avcodec_alloc_frame();
		if(pFrameRGB==NULL)
			return -1;

		FILE* stream_info = NULL;
		stream_info = fopen("stream_info.dat","wb");

		if(stream_info)
		{
			uint32_t word_out = 0;// = pFormatCtx->streams[i]->codec->
			word_out = pFormatCtx->streams[videoStream]->time_base.num;
			fwrite(&word_out,sizeof(word_out),1,stream_info);
			word_out = pFormatCtx->streams[videoStream]->time_base.den;
			fwrite(&word_out,sizeof(word_out),1,stream_info);			
			word_out = pFormatCtx->streams[videoStream]->duration;
			fwrite(&word_out,sizeof(word_out),1,stream_info);
			word_out = pCodecCtx->coded_width;
			fwrite(&word_out,sizeof(word_out),1,stream_info);
			word_out = pCodecCtx->coded_height;
			fwrite(&word_out,sizeof(word_out),1,stream_info);
			fclose(stream_info);
		}
		// Determine required buffer size and allocate buffer
		numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
			pCodecCtx->height);
		buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

		sws_ctx =
			sws_getContext
			(
			pCodecCtx->width,
			pCodecCtx->height,
			pCodecCtx->pix_fmt,
			pCodecCtx->width,
			pCodecCtx->height,
			PIX_FMT_RGB24,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL
			);

			frame_data_size = pCodecCtx->width * pCodecCtx->height * 3;
		// Assign appropriate parts of buffer to image planes in pFrameRGB
		// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
		// of AVPicture
		avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);

		//AllocateBlockBuffer(pCodecCtx->height,pCodecCtx->width);

		ReadStream(frame_start, mode);
		//ReadStream(frame_start);

		if(!mode) {
			
			//Colour::canvas_image canvas_ = global_rgb_hist_.canvas();
			Colour::canvas_image canvas_ = global_rgb_hist_.hilbert_canvas();
			std::string file_out = "canvas.bmp";
			WriteBitmap(file_out,canvas_.width,canvas_.height,canvas_.planes,canvas_.data);

			canvas_ = global_rgb_hist_.hilbert_sorted_canvas();
			file_out = "sorted_canvas.bmp";
			WriteBitmap(file_out,canvas_.width,canvas_.height,canvas_.planes,canvas_.data);
			std::string hist_filename = "hist.dat";
			global_rgb_hist_.save(hist_filename,(uint32_t)pCodecCtx->width,(uint32_t)pCodecCtx->height);
		}
		else if(mode == 3)
		{
			if(frame_data_size >= 11520000)
			{
				select_frames = 50;
				printf("Select frame set to 50! [%dx%d]\n",pCodecCtx->width, pCodecCtx->height);
			}

			global_rgb_hist_.select_best_frames(select_frames);
			global_rgb_hist_.select_best_frames_thumb((unsigned)THUMBNAIL_SELECT);
		}

		frame_start.shrink_to_fit();

		frame_count = (uint64)frame_start.size();
		

		FramePointer frame_pointer;
		//ReadFrame(29,frame_pointer);
		//WriteToFile();
		video_stream_state.buffer = buffer;
		video_stream_state.frameFinished = frameFinished;
		video_stream_state.numBytes = numBytes;
		video_stream_state.optionsDict = optionsDict;
		video_stream_state.packet = packet;
		video_stream_state.pCodec = pCodec;
		video_stream_state.pCodecCtx = pCodecCtx;
		video_stream_state.pFormatCtx = pFormatCtx;
		video_stream_state.pFrame = pFrame;
		video_stream_state.pFrameRGB = pFrameRGB;
		video_stream_state.sws_ctx = sws_ctx;
		video_stream_state.videoStream = videoStream;		
		video_stream_state.frame_start = frame_start;
		//read_tid = SDL_CreateThread(ReadThread, "read_thread",this); 
		//read_prev_tid = SDL_CreateThread(ReadPrevThread, "read_prev_thread",this);
		/*SDL_Delay(1000);
		ReadFrame(240,frame_pointer);
		SDL_Delay(1000);
		ReadFrame(28,frame_pointer);*/

		return 0;

}

uint64 FrameServer::Length() {
	return frame_count;
}


void FrameServer::RGBToGray(pixel* rgb_ptr, pixel* gray_ptr,size_t width,size_t height) {
	size_t stride = width*3;
	unsigned int pel = 0;

	for(size_t y = 0; y < height; y++) {
		pixel* rgb_row_ptr = rgb_ptr + y*stride;
		pixel* gray_row_ptr = gray_ptr + y*width;

		for(size_t x = 0; x < width; x++) {
			pel = *(rgb_row_ptr)*2989 + *(rgb_row_ptr+1)*5870 + *(rgb_row_ptr+2)*1140;
			*gray_row_ptr = pixel(pel / 10000);

			pel = pel % 10000;

			if(pel > 5000)
				(*gray_row_ptr)++;
			//*g_ptr = REDF(c_ptr)*0.2989 + GRNF(c_ptr)*0.587 + BLUF(c_ptr)*0.114;

			rgb_row_ptr += 3;
			gray_row_ptr++;
		}
	}
}



void FrameServer::SaveFrame(AVFrame* pFrameY,AVFrame *pFrame, size_t width, size_t height, 
	long long iFrame,FramePointer &frame_pointer,bool run_ip,FrameServer* server) {
		/*FramePointer frame_pointer;
		GetFramePointer(uint64(iFrame), frame_pointer);*/
		//frame_pointer.video_frame_ptr->SetFrameNumber(uint64(iFrame));
		pixel* rgb_ptr = frame_pointer.rgb_ptr;
		pixel* y_ptr = frame_pointer.y_ptr;
		size_t stride = width*3;
		size_t pixel_count = width*height;

		
		// Write pixel data
		for(size_t y = 0; y < height; y++) {
			memcpy(rgb_ptr,pFrame->data[0]+y*pFrame->linesize[0],stride);
			memcpy(y_ptr,pFrameY->data[0]+y*pFrameY->linesize[0],width);
			rgb_ptr += stride;
			y_ptr += width;
			//fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
		}

		//RGBToGray(frame_pointer.rgb_ptr,frame_pointer.y_ptr,width,height);		
}

void FrameServer::ReadStream(std::vector<uint64> &frame_start, int mode) {
	frame_start.clear();
	frame_start.reserve(1000000);
	avcodec_flush_buffers(pCodecCtx);
	av_seek_frame(pFormatCtx,videoStream, 0, AVSEEK_FLAG_BACKWARD);
	int i = 0;
	uint64 packet_count = 0;
	VideoFrame vid_frame;
	vid_frame.Allocate(pCodecCtx->width * pCodecCtx->height);
	FramePointer frame_pointer;
	frame_pointer.rgb_ptr = vid_frame.rgbPointer();
	frame_pointer.y_ptr = vid_frame.yPointer();

	bool begin = true;
	bool found_colour = false;
	char file_name_out[256];
	bool end_of_read = false;
	bool end_of_read_thumb = false;
	bool converted = false;
	uint32_t rank = 0;

	while(av_read_frame(pFormatCtx, &packet)>=0) {
		// Is this a packet from the video stream?
		if(packet.stream_index==videoStream) {
			//fwrite(&packet,sizeof(AVPacket),1,packet_file);
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			if(begin) {
				frame_start.push_back(packet.pts);
				begin = false;
			}

			// Did we get a video frame?
			if(frameFinished) {
				// Convert the image from its native format to RGB
				int result = sws_scale
				(
				sws_ctx,
				pFrame->data,
				pFrame->linesize,
				0,
				pCodecCtx->height,
				pFrameRGB->data,
				pFrameRGB->linesize
				);


				// Save the frame to disk
				//if((i >= 10) && (i <= 10)) {
				
					

					if(!mode) {
						SaveFrame(pFrame,pFrameRGB, pCodecCtx->width, pCodecCtx->height, i, frame_pointer);
						/*frame_rgb_hist_.clear();
						frame_rgb_hist_.update(frame_pointer.rgb_ptr, pCodecCtx->width*pCodecCtx->height);*/
						found_colour = global_rgb_hist_.update(frame_pointer.rgb_ptr, pCodecCtx->width*pCodecCtx->height);				

						//rgb2bgr(frame_pointer.rgb_ptr, pCodecCtx->width,pCodecCtx->height);

						//if(found_colour)
						//{
						//	std::string file_out = "temp.bmp";
						//	WriteBitmap(file_out,pCodecCtx->width,pCodecCtx->height,3,frame_pointer.rgb_ptr);
						//}

						//if(i == 10) {
						//	//break;
						//	//Colour::canvas_image canvas_ = global_rgb_hist_.canvas();
						//	//file_out = "canvas.bmp";
						//	//WriteBitmap(file_out,canvas_.width,canvas_.height,canvas_.planes,canvas_.data);
						//}
					}
					else if(mode == 3)
					{
						SaveFrame(pFrame,pFrameRGB, pCodecCtx->width, pCodecCtx->height, i, frame_pointer);					
						found_colour = global_rgb_hist_.score(frame_pointer.rgb_ptr, pCodecCtx->width*pCodecCtx->height);	
					}
					else if(mode == 4)
					{
						converted = false;

						if(global_rgb_hist_.save_this_frame(i,end_of_read, rank))
						{
							SaveFrame(pFrame,pFrameRGB, pCodecCtx->width, pCodecCtx->height, i, frame_pointer);
							rgb2bgr(frame_pointer.rgb_ptr, pCodecCtx->width,pCodecCtx->height);
							converted = true;
							sprintf(file_name_out,"dump/frame%05d_%02u.bmp",i,rank);
							WriteBitmap(std::string(file_name_out),pCodecCtx->width,pCodecCtx->height,3,frame_pointer.rgb_ptr);

							if(end_of_read && end_of_read_thumb)
								break;
						}

						if(global_rgb_hist_.save_this_frame_thumb(i,end_of_read_thumb, rank))
						{
							if(!converted) {
								SaveFrame(pFrame,pFrameRGB, pCodecCtx->width, pCodecCtx->height, i, frame_pointer);
								rgb2bgr(frame_pointer.rgb_ptr, pCodecCtx->width,pCodecCtx->height);
							}

							sprintf(file_name_out,"dumpt/frame%05d_%02u.bmp",i,rank);
							WriteBitmap(std::string(file_name_out),pCodecCtx->width,pCodecCtx->height,3,frame_pointer.rgb_ptr);

							if(end_of_read && end_of_read_thumb)
								break;
						}
					}
					else if(!(i % 100))
					{
						SaveFrame(pFrame,pFrameRGB, pCodecCtx->width, pCodecCtx->height, i, frame_pointer);
						rgb2bgr(frame_pointer.rgb_ptr, pCodecCtx->width,pCodecCtx->height);
						sprintf(file_name_out,"dump\\frame%05d.bmp",i);
						WriteBitmap(std::string(file_name_out),pCodecCtx->width,pCodecCtx->height,3,frame_pointer.rgb_ptr);
					}

				begin = true;
				i++;
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		packet_count++;
	}
}

void FrameServer::rgb2bgr(pixel* rgb_ptr, size_t width,size_t height)
{
	size_t pixels = width*height;

	bgra* p_pix = (bgra*)rgb_ptr;
	pixel red = 0;

	for(size_t p = 0; p < pixels; ++p)
	{
		red = p_pix->r;
		p_pix->r = p_pix->b;
		p_pix->b = red;
		rgb_ptr += 3;
		p_pix = (bgra*)rgb_ptr;
	}
}

#define PP_WORD short
#define PP_DWORD int

void FrameServer::WriteBitmap(std::string filename,int _width,int _height,int planes,uint8_t* dataPtr)
{
	FILE* bitmapFile;
	PP_WORD Type=19778;
	PP_DWORD Size;
	PP_DWORD Reserved = 0;
	PP_DWORD Offset = 54;
	PP_DWORD headerSize = 40;
	PP_DWORD Width = _width;
	PP_DWORD Height = _height;
	PP_WORD Planes = 1;
	PP_WORD BitsPerPixel = 8*planes;
	PP_DWORD Compression = 0;
	PP_DWORD SizeImage;
	PP_DWORD XPixelsPerMeter = 0;
	PP_DWORD YPixelsPerMeter = 0;
	PP_DWORD ColorsUsed = 0;
	PP_DWORD ColorsImportant = 0;

	int stride = Width*(BitsPerPixel/8);
	int bytesPerLine = Width*(BitsPerPixel/8)*sizeof(uint8_t);
	int pad = stride % 4;


	if(pad != 0)
	{
		stride+=(4 - pad);
	}

	SizeImage = stride*Height;
	Size = SizeImage + Offset;
	uint8_t* writeBuffer = new uint8_t[stride];

	bitmapFile = fopen(filename.c_str(),"wb");

	fwrite((const void*)&Type,sizeof(PP_WORD),1,bitmapFile);		
	fwrite((const void*)&Size,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&Reserved,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&Offset,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&headerSize,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&Width,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&Height,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&Planes,sizeof(PP_WORD),1,bitmapFile);
	fwrite((const void*)&BitsPerPixel,sizeof(PP_WORD),1,bitmapFile);
	fwrite((const void*)&Compression,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&SizeImage,sizeof(PP_DWORD),1,bitmapFile);

	fwrite((const void*)&XPixelsPerMeter,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&YPixelsPerMeter,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&ColorsUsed,sizeof(PP_DWORD),1,bitmapFile);
	fwrite((const void*)&ColorsImportant,sizeof(PP_DWORD),1,bitmapFile);



	int pixelCount = _width*_height*planes;

	uint8_t* linePtr = dataPtr+((Height-1)*bytesPerLine);
	//fwrite((void*)dataPtr,sizeof(uint8_t),pixelCount,bitmapFile);
	for(int row = 0; row < Height; row++)
	{
		memcpy((void*)writeBuffer,(const void*)linePtr,bytesPerLine);		
		fwrite((void*)linePtr,sizeof(uint8_t),stride,bitmapFile);
		linePtr -= bytesPerLine;
	}

	delete[] writeBuffer;
	fclose(bitmapFile);
}

void FrameServer::ReadBitmap(std::string filename,int &width, int &height, int &planes, std::vector<uint8_t> &bitmap_image)
{
	FILE* bitmapFile;
	PP_WORD Type;
	PP_DWORD Size;
	PP_DWORD Reserved;
	PP_DWORD Offset;
	PP_DWORD headerSize;
	PP_DWORD Width;
	PP_DWORD Height;
	PP_WORD Planes;
	PP_WORD BitsPerPixel;
	PP_DWORD Compression;
	PP_DWORD SizeImage;
	PP_DWORD XPixelsPerMeter;
	PP_DWORD YPixelsPerMeter;
	PP_DWORD ColorsUsed;
	PP_DWORD ColorsImportant;

	bitmapFile = fopen(filename.c_str(),"rb");
	fread((void*)&Type,sizeof(PP_WORD),1,bitmapFile);
	fread((void*)&Size,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&Reserved,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&Offset,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&headerSize,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&Width,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&Height,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&Planes,sizeof(PP_WORD),1,bitmapFile);
	fread((void*)&BitsPerPixel,sizeof(PP_WORD),1,bitmapFile);
	fread((void*)&Compression,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&SizeImage,sizeof(PP_DWORD),1,bitmapFile);

	fread((void*)&XPixelsPerMeter,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&YPixelsPerMeter,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&ColorsUsed,sizeof(PP_DWORD),1,bitmapFile);
	fread((void*)&ColorsImportant,sizeof(PP_DWORD),1,bitmapFile);


	width = Width;
	height = Height;
	planes = BitsPerPixel/8;

	int stride = Width*(BitsPerPixel>>3);
	int bytesPerLine = Width*(BitsPerPixel>>3)*sizeof(uint8_t);
	int pad = stride % 4;

	bitmap_image.resize(bytesPerLine*Height);
	uint8_t* dataPtr = &bitmap_image[0];

	if(pad != 0)
	{
		stride+=(4 - pad);
	}
	std::vector<uint8_t> buffer(stride);
	uint8_t* readBuffer = &buffer[0];

	fseek(bitmapFile,Offset,SEEK_SET);
	
	uint8_t* linePtr = dataPtr+((Height-1)*bytesPerLine);
	//fwrite((void*)dataPtr,sizeof(uint8_t),pixelCount,bitmapFile);
	for(int row = 0; row < Height; row++)
	{
		fread((void*)readBuffer,sizeof(uint8_t),stride,bitmapFile);
		memcpy((void*)linePtr,(const void*)readBuffer,bytesPerLine);
		linePtr -= bytesPerLine;
	}

	fclose(bitmapFile);	
}