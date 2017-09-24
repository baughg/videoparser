#ifndef RGB_HISTOGRAM_H
#define RGB_HISTOGRAM_H

#include <string>
#include <string.h>
#include "histogram.h"

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
}

class RGBHistogram :	public Histogram
{
public:
	RGBHistogram(void);
	~RGBHistogram(void);
	bool update(uint8_t* rgb_ptr, uint32_t count);
	void save(std::string out_name);
	void load(std::string out_name);
	void clear();
	Colour::canvas_image & canvas();
	Colour::canvas_image & sorted_canvas();
	Colour::canvas_image & make_colour_groups();
protected:
	void init();
	std::vector<uint32_t> canvas_data_;;
	Colour::canvas_image canvas_image_;
};
#endif

