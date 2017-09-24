#include "frame_server.h"
#include <stdio.h>
#include <stdlib.h>


int main()
{
	std::string src_file = "bolt.mp4";

	FrameServer frame_server;

	if (frame_server.Parse(src_file,2) != 0)
	{
		printf("decode error!\n");
	}
	return 0;
}