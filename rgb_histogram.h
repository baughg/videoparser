#ifndef RGB_HISTOGRAM_H
#define RGB_HISTOGRAM_H

#include "histogram.h"
#include <string>

namespace Colour
{
	typedef struct rgba
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	}rgba;

	typedef struct bgra
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	}bgra;

	typedef struct canvas_image
	{
		uint32_t width;
		uint32_t height;
		uint32_t planes;
		uint8_t* data;
		uint32_t occupied;
	}canvas_image;

	typedef struct histogram_file_header
	{
		histogram_file_header()
		{
			colours = 0;
			width = 0;
			height = 0;
			frames = 0;
		}

		uint32_t colours;
		uint32_t width;
		uint32_t height;
		uint32_t frames;
	}histogram_file_header;

	typedef struct frame_score
	{
		double score;
		uint32_t frame;
	}frame_score;

	typedef struct frame_rank
	{
		frame_rank()
		{
		}
		uint32_t rank;
		uint32_t frame;
	}frame_rank;
}

class RGBHistogram :	public Histogram
{
public:
	RGBHistogram(void);
	~RGBHistogram(void);
	bool update(uint8_t* rgb_ptr, uint32_t count);
	bool score(uint8_t* rgb_ptr, uint32_t count);
	void save(std::string out_name, uint32_t width = 0, uint32_t height = 0);
	void load(std::string out_name);
	void clear();
	void select_best_frames(const uint32_t frames_chosen);
	void init_frame_scoring();
	void init_frame_selection();
	Colour::canvas_image & canvas();
	Colour::canvas_image & hilbert_canvas();
	Colour::canvas_image & sorted_canvas();
	Colour::canvas_image & hilbert_sorted_canvas();
	Colour::canvas_image & make_colour_groups();
	bool save_this_frame(const uint32_t &frame, bool &end_read, uint32_t &rank);
protected:
	bool save_frame_rank();
	bool load_frame_rank();
	void log_probability();
	void init();
	std::vector<uint32_t> canvas_data_;
	Colour::canvas_image canvas_image_;
	Colour::histogram_file_header histogram_file_header_;
	std::vector<Colour::frame_score> frame_scores_;
	std::vector<Colour::frame_score> frame_selected_;
	uint32_t frame_no_;
	uint32_t select_frame_count_;
	uint32_t score_width_;
	uint32_t score_height_;
	std::vector<Colour::frame_rank> frame_ranks_;
};
#endif

