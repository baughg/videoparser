#include "frame_server.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char** argv)
{
	if(argc < 2)
	{
		printf("decode [video]\n");
		exit(1);		
	}

	std::string src_file = std::string(argv[1]);
	
	{
		FrameServer frame_server;
		printf("create histogram...\n");
		if (frame_server.Parse(src_file) != 0)
		{
			printf("decode error!\n");
		}
	}
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
	return 0;
}