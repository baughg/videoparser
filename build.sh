rm decoder
g++ -O2 -std=c++11 video_frame.cpp frame_server.cpp histogram.cpp rgb_histogram.cpp decoder.cpp -lavformat -lavcodec -lswscale -lavutil -lavfilter -lswresample -lavdevice -lpostproc -lx264 -lrt -o decoder
