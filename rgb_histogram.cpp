#include "rgb_histogram.h"
#include "hilbert_curve2d.h"
#include <algorithm>
#include <stdio.h>

RGBHistogram::RGBHistogram(void)
{
	init();
}


RGBHistogram::~RGBHistogram(void)
{
}

bool frequency_sort(const colour_frequency &lhs, const colour_frequency &rhs)
{
	return lhs.freq > rhs.freq;
}

void RGBHistogram::init()
{
	hist_.resize(1 << 24);
	canvas_data_.resize(hist_.size());
	canvas_image_.height = 4096;
	canvas_image_.width = 4096;
	canvas_image_.planes = 4;
	canvas_image_.data = (uint8_t*)&canvas_data_[0];
}

void RGBHistogram::clear()
{
	memset(&hist_[0],0,hist_.size()*sizeof(hist_[0]));
}

Colour::canvas_image & RGBHistogram::hilbert_canvas()
{
	const uint32_t colours = canvas_image_.height * canvas_image_.width;
	Colour::rgba colour;
	uint32_t * p_colour;
	p_colour = (uint32_t *)&colour;
	colour.a = 0;
	std::vector<uint32_t> colour_lookup;

	FILE* rgb_file = NULL;

	rgb_file = fopen("rgb_line.dat", "rb");


	if (!rgb_file)
		return canvas_image_;

	colour_lookup.resize(1 << 24);
	fread(&colour_lookup[0], sizeof(uint32_t), colour_lookup.size(), rgb_file);
	fclose(rgb_file);

	Colour::bgra* p_bgra = (Colour::bgra*)canvas_image_.data;
	Colour::bgra* p_bgra_0 = p_bgra;
	canvas_image_.occupied = 0;
	int order = 0;

	for (uint32_t y = 0; y < canvas_image_.width; ++y)
	{
		for (uint32_t x = 0; x < canvas_image_.width; ++x)
		{
			order = xy2d(12, x, y);
			*p_colour = colour_lookup[order];

			if (hist_[*p_colour])
			{
				p_bgra->r = colour.r;
				p_bgra->g = colour.g;
				p_bgra->b = colour.b;
				p_bgra->a = 0xff;
				canvas_image_.occupied++;
			}
			else
			{
				p_bgra->r = 0;
				p_bgra->g = 0;
				p_bgra->b = 0;
			}

			p_bgra++;
		}
	}

	return canvas_image_;
}

Colour::canvas_image & RGBHistogram::hilbert_sorted_canvas()
{
	const uint32_t colours = canvas_image_.height * canvas_image_.width;
	Colour::rgba colour;
	uint32_t * p_colour;
	p_colour = (uint32_t *)&colour;
	colour.a = 0;


	colour_frequency_.resize(hist_.size());
	unsigned col = 0;

	for (uint32_t red = 0; red < 256; ++red)
	{
		colour.r = red;

		for (uint32_t green = 0; green < 256; ++green) {
			colour.g = green;
			for (uint32_t blue = 0; blue < 256; ++blue)
			{
				colour.b = blue;
				col = *p_colour;
				colour_frequency_[col].freq = hist_[col];
				colour_frequency_[col].colour = col;
			}
		}
	}

	std::sort(colour_frequency_.begin(), colour_frequency_.end(), frequency_sort);

	Colour::bgra* p_bgra = (Colour::bgra*)canvas_image_.data;
	Colour::bgra* p_bgra_0 = p_bgra;
	canvas_image_.occupied = 0;
	Colour::rgba* p_rgba_out;


	memset(p_bgra, 0, colours * sizeof(Colour::bgra));
	int x = 0, y = 0, index = 0;

	for (uint32_t c = 0; c < colours; ++c)
	{
		if (!colour_frequency_[c].freq)
			break;

		d2xy(12, c, x, y);
		index = (y * canvas_image_.width) + x;
		p_bgra = p_bgra_0 + index;
		p_rgba_out = (Colour::rgba*)&colour_frequency_[c].colour;
		p_bgra->r = p_rgba_out->r;
		p_bgra->g = p_rgba_out->g;
		p_bgra->b = p_rgba_out->b;
		canvas_image_.occupied++;
	}

	colour_frequency_.resize(canvas_image_.occupied);
	return canvas_image_;
}

Colour::canvas_image & RGBHistogram::canvas()
{
	const uint32_t colours = canvas_image_.height * canvas_image_.width;
	Colour::rgba colour;
	uint32_t * p_colour;
	p_colour = (uint32_t *)&colour;
	colour.a = 0;

	Colour::bgra* p_bgra = (Colour::bgra*)canvas_image_.data;
	Colour::bgra* p_bgra_0 = p_bgra;
	canvas_image_.occupied = 0;
	for(uint32_t red = 0; red < 256; ++red)
	 {
		colour.r = red;
		p_bgra = p_bgra_0 + ((red >> 4 )*256*canvas_image_.width) + (red%16)*256;
		//p_bgra = p_bgra_0 + (blue%16)*256 + (15*256*canvas_image_.width);

		for(uint32_t green = 0; green < 256; ++green) {
			colour.g = green;		
			for(uint32_t blue = 0; blue < 256; ++blue)			
			{
				colour.b = blue;
				

				if(hist_[*p_colour])
				{
					p_bgra[blue].r = (uint8_t)red;
					p_bgra[blue].g = (uint8_t)green;
					p_bgra[blue].b = (uint8_t)blue;			
					canvas_image_.occupied++;
				}
				else
				{
					p_bgra[blue].r = 0;
					p_bgra[blue].g = 0;
					p_bgra[blue].b = 0;		
				}				
			}

			p_bgra += canvas_image_.width;
		}
	}

	return canvas_image_;
}



void RGBHistogram::save(std::string out_name)
{
	uint32_t colours = (uint32_t)colour_frequency_.size();

	FILE* hist_file = nullptr;

	hist_file = fopen(out_name.c_str(),"wb");

	if(hist_file)
	{
		fwrite(&colours,sizeof(uint32_t),1,hist_file);
		fwrite(&colour_frequency_[0],sizeof(colour_frequency),colours,hist_file);
		fclose(hist_file);
	}
}

bool sort_by_group(const colour_group &lhs, const colour_group &rhs)
{
	return lhs.group < rhs.group;
}


void RGBHistogram::load(std::string out_name)
{
	uint32_t colours = 0;
	
	FILE* hist_file = nullptr;

	hist_file = fopen(out_name.c_str(),"rb");

	if(hist_file)
	{
		fread(&colours,sizeof(uint32_t),1,hist_file);

		if(colours){
			colour_frequency_.resize(colours);
			fread(&colour_frequency_[0],sizeof(colour_frequency),colours,hist_file);
		}
		fclose(hist_file);
	}
}

Colour::canvas_image & RGBHistogram::make_colour_groups()
{
	size_t colours = colour_frequency_.size();

	colour_group_.resize(colours);

	for(size_t c = 0; c < colours; ++c)
	{
		colour_group_[c].col_freq = colour_frequency_[c];
	}

	Colour::rgba *p_colour_a;
	Colour::rgba *p_colour_b;
	int32_t col_diff = 0;
	int32_t diff = 0;
	uint32_t group = 1;
	uint32_t group_size = 0;

	for(size_t c = 0; c < colours; ++c)
	{
		if(colour_group_[c].group)
			continue;

		colour_group_[c].group = group++;
		group_size = 1;

		p_colour_a = (Colour::rgba*)&colour_group_[c].col_freq.colour;

		for(size_t o = c+1; o < colours; ++o)
		{
			p_colour_b = (Colour::rgba*)&colour_group_[o].col_freq.colour;

			col_diff = (p_colour_a->r - p_colour_b->r);
			diff = col_diff * col_diff;
			col_diff = (p_colour_a->b - p_colour_b->b);
			diff += (col_diff * col_diff);
			col_diff = (p_colour_a->g - p_colour_b->g);
			diff += (col_diff * col_diff);

			if(diff <= 900)
			{
				colour_group_[o].group = colour_group_[c].group;
				group_size++;
			}
		}
	}

	// sort by group
	std::sort(colour_group_.begin(),colour_group_.end(),sort_by_group);

	Colour::bgra* p_bgra = (Colour::bgra*)canvas_image_.data;
	Colour::bgra* p_bgra_0 = p_bgra;
	canvas_image_.occupied = 0;
	Colour::rgba* p_rgba_out;
	

	memset(p_bgra,0,colours*sizeof(Colour::bgra));

	for(uint32_t c = 0; c < colours; ++c)
	{		
		p_rgba_out = (Colour::rgba*)&colour_group_[c].col_freq.colour;
		p_bgra->r = p_rgba_out->r;
		p_bgra->g = p_rgba_out->g;
		p_bgra->b = p_rgba_out->b;
		p_bgra++;
		canvas_image_.occupied++;
	}

	return canvas_image_;
}


Colour::canvas_image & RGBHistogram::sorted_canvas()
{
	const uint32_t colours = canvas_image_.height * canvas_image_.width;
	Colour::rgba colour;
	uint32_t * p_colour;
	p_colour = (uint32_t *)&colour;
	colour.a = 0;

	
	colour_frequency_.resize(hist_.size());
	unsigned col = 0;

	for(uint32_t red = 0; red < 256; ++red)
	 {
		colour.r = red;
		
		for(uint32_t green = 0; green < 256; ++green) {
			colour.g = green;		
			for(uint32_t blue = 0; blue < 256; ++blue)			
			{
				colour.b = blue;
				col = *p_colour;
				colour_frequency_[col].freq = hist_[col];
				colour_frequency_[col].colour = col;
			}			
		}
	}

	std::sort(colour_frequency_.begin(),colour_frequency_.end(),frequency_sort);

	Colour::bgra* p_bgra = (Colour::bgra*)canvas_image_.data;
	Colour::bgra* p_bgra_0 = p_bgra;
	canvas_image_.occupied = 0;
	Colour::rgba* p_rgba_out;
	

	memset(p_bgra,0,colours*sizeof(Colour::bgra));

	for(uint32_t c = 0; c < colours; ++c)
	{
		if(!colour_frequency_[c].freq)
			break;

		p_rgba_out = (Colour::rgba*)&colour_frequency_[c].colour;
		p_bgra->r = p_rgba_out->r;
		p_bgra->g = p_rgba_out->g;
		p_bgra->b = p_rgba_out->b;
		p_bgra++;
		canvas_image_.occupied++;
	}

	colour_frequency_.resize(canvas_image_.occupied);
	return canvas_image_;
}

bool RGBHistogram::update(uint8_t* rgb_ptr, uint32_t count)
{
	Colour::rgba* p_rgba = (Colour::rgba*)rgb_ptr;
	Colour::rgba colour;
	colour.a = 0;
	uint32_t * p_colour;
	p_colour = (uint32_t *)&colour;
	bool found_colour = false;

	for(uint32_t c = 0; c < count; ++c)
	{
		colour.r = p_rgba->r;
		colour.g = p_rgba->g;
		colour.b = p_rgba->b;

		/*if(p_rgba->b == 255 && p_rgba->r == 0 && p_rgba->g == 0)
			found_colour = true;*/

		hist_[*p_colour]++;
		rgb_ptr += 3;
		p_rgba = (Colour::rgba*)rgb_ptr;
	}

	return found_colour;
}
