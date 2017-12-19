#include "frame_server.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char** argv)
{
	if(argc < 3)
	{
		printf("decode [video]\n");
		exit(1);		
	}

	std::string src_file = std::string(argv[2]);
	std::string mode = std::string(argv[1]);
	if(strcmp(mode.c_str(),"hist") == 0) {
		{
			FrameServer frame_server;
			printf("create histogram...\n");
			if (frame_server.Parse(src_file) != 0)
			{
				printf("decode error!\n");
			}
		}		
	} 
	else if(strcmp(mode.c_str(),"rank") == 0)
	{
		printf("rank frames...\n");
		std::string hist_filename = "hist.dat";
		FrameServer local_server;
		RGBHistogram &global_hist = local_server.get_global_histogram();
		global_hist.load(hist_filename);
		global_hist.init_frame_scoring();		

		if (local_server.Parse(src_file, 3) != 0)
		{
			printf("decode error!\n");
		}
	}
	else if(strcmp(mode.c_str(),"thumb") == 0)
	{
		printf("write selected frames...\n");
		FrameServer local_server;			
		RGBHistogram &global_hist = local_server.get_global_histogram();
		global_hist.init_frame_selection();		

		if (local_server.Parse(src_file, 4) != 0)
		{
			printf("decode error!\n");
		}
	}
	else if(strcmp(mode.c_str(),"face") == 0)
	{
		printf("face detection on selected frames...\n");
		FrameServer local_server;			
		RGBHistogram &global_hist = local_server.get_global_histogram();
		global_hist.init_frame_selection();		

		if(!global_hist.generate_face_detection_script("seeta_fd_frontal_v1.0.bin"))		
			printf("face detection script error!\n");
		
	}
	else if(strcmp(mode.c_str(),"facepull") == 0)
	{
		printf("face extraction on selected frames...\n");
		FrameServer local_server;			
		RGBHistogram &global_hist = local_server.get_global_histogram();
		global_hist.init_frame_selection();		

		if(!global_hist.extract_face_bitmaps())		
			printf("face extraction script error!\n");
		
	}
	else if(strcmp(mode.c_str(),"facesel") == 0)
	{
		printf("face selection...\n");				
		

		if(RGBHistogram::select_final_faces())		
			printf("face extraction script error!\n");
		
	}
	return 0;
}