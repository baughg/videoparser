#include "rgb_histogram.h"
#include "hilbert_curve2d.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <fstream>
#include "frame_server.h"



RGBHistogram::RGBHistogram(void)
{
	init();
}


RGBHistogram::~RGBHistogram(void)
{
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

	FILE* rgb_file = nullptr;

	rgb_file = fopen("rgb_line.dat","rb");


	if(!rgb_file)
		return canvas_image_;

	colour_lookup.resize(1 << 24);
	fread(&colour_lookup[0],sizeof(uint32_t),colour_lookup.size(),rgb_file);
	fclose(rgb_file);

	Colour::bgra* p_bgra = (Colour::bgra*)canvas_image_.data;
	Colour::bgra* p_bgra_0 = p_bgra;
	canvas_image_.occupied = 0;
	int order = 0;

	for(uint32_t y = 0; y < canvas_image_.width; ++y)
	{
		for(uint32_t x = 0; x < canvas_image_.width; ++x)
		{
			order = xy2d(12, x, y);      
      *p_colour = colour_lookup[order];

				if(hist_[*p_colour])
				{
					p_bgra->r	= colour.r;
					p_bgra->g	= colour.g;
					p_bgra->b	= colour.b;
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

bool frequency_sort(const colour_frequency &lhs, const colour_frequency &rhs)
{
	return lhs.freq > rhs.freq;
}

void RGBHistogram::save(std::string out_name, uint32_t width, uint32_t height)
{
	uint32_t colours = (uint32_t)colour_frequency_.size();

	FILE* hist_file = nullptr;

	hist_file = fopen(out_name.c_str(),"wb");
	histogram_file_header_.width = width;
	histogram_file_header_.height = height;
	histogram_file_header_.colours = colours;

	if(hist_file)
	{
		fwrite(&histogram_file_header_,sizeof(histogram_file_header_),1,hist_file);
		fwrite(&colour_frequency_[0],sizeof(colour_frequency),colours,hist_file);
		fclose(hist_file);
	}
}

bool sort_by_group(const colour_group &lhs, const colour_group &rhs)
{
	return lhs.group < rhs.group;
}

void RGBHistogram::init_frame_scoring()
{
	log_probability();
	frame_scores_.resize(histogram_file_header_.frames);
	frame_no_ = 0;
	score_height_ = histogram_file_header_.height - 1;
	score_width_ = histogram_file_header_.width - 1;
}

void RGBHistogram::init_frame_selection()
{
	load_frame_rank();
	load_frame_rank(true);
	frame_no_ = 0;
	frame_no_thumb_ = 0;
	select_frame_count_ = (uint32_t)frame_ranks_.size();
	select_frame_count_thumb_ = (uint32_t)frame_ranks_thumb_.size();
}

bool RGBHistogram::save_this_frame(const uint32_t &frame, bool &end_read, uint32_t &rank)
{
	bool found = false;
	
	if(frame_no_ >= select_frame_count_)
	{
		end_read = true;
		return false;
	}

	if(frame_ranks_[frame_no_].frame == frame)
	{
		rank = frame_ranks_[frame_no_].rank;
		frame_no_++;
		found = true;
	}

	end_read = frame_no_ >= select_frame_count_;
		
	return found;
}

bool RGBHistogram::save_this_frame_thumb(const uint32_t &frame, bool &end_read, uint32_t &rank)
{
	bool found = false;
	
	if(frame_no_thumb_ >= select_frame_count_thumb_)
	{
		end_read = true;
		return false;
	}

	if(frame_ranks_thumb_[frame_no_thumb_].frame == frame)
	{
		rank = frame_ranks_thumb_[frame_no_thumb_].rank;
		frame_no_thumb_++;
		found = true;
	}

	end_read = frame_no_thumb_ >= select_frame_count_thumb_;
		
	return found;
}


void RGBHistogram::log_probability()
{
	const size_t colours = colour_frequency_.size();

	if(!colours)
		return;

	const size_t count = 1ULL << 24;

	log_probability_.resize(count);

	for(size_t col = 0; col < count; ++col)
	{
		log_probability_[col] = 1.0e20;
	}

	uint64_t pixel_count = 0ULL;
	double log_pixel_count = 0.0;
	double log_freq = 0.0;
	size_t colour = 0;

	for(size_t col = 0; col < colours; ++col)
	{
		colour = colour_frequency_[col].colour;
		log_freq = std::log((double)colour_frequency_[col].freq);
		log_probability_[colour] = log_freq;
		pixel_count += colour_frequency_[col].freq;
	}

	log_pixel_count = std::log((double)pixel_count);

	for(size_t col = 0; col < colours; ++col)
	{
		colour = colour_frequency_[col].colour;
		log_freq = log_probability_[colour];
		log_freq -= log_pixel_count;
		log_probability_[colour] = -log_freq;		
	}
}

void RGBHistogram::load(std::string out_name)
{
	uint32_t colours = 0;
	
	FILE* hist_file = nullptr;

	hist_file = fopen(out_name.c_str(),"rb");

	if(hist_file)
	{
		fread(&histogram_file_header_,sizeof(histogram_file_header_),1,hist_file);
		colours = histogram_file_header_.colours;

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

Colour::canvas_image & RGBHistogram::hilbert_sorted_canvas()
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
	int x = 0, y = 0, index = 0;

	for(uint32_t c = 0; c < colours; ++c)
	{
		if(!colour_frequency_[c].freq)
			break;

		d2xy(12,c,x,y);
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

bool sort_by_score(const Colour::frame_score &lhs, const Colour::frame_score &rhs)
{
	return lhs.score > rhs.score;
}

bool sort_by_frame_no(const Colour::frame_rank &lhs, const Colour::frame_rank &rhs)
{
	return lhs.frame < rhs.frame;
}

void RGBHistogram::select_best_frames(const uint32_t frames_chosen)
{
	if(!frames_chosen)
		return;

	std::sort(frame_scores_.begin(),frame_scores_.end(),sort_by_score);
	uint32_t last_frame = ~0;
	uint32_t frame_distance = 0;
	const uint32_t frame_gap = histogram_file_header_.frames / frames_chosen;
	const int32_t half_gap = (int)frame_gap >> 1;

	frame_selected_.resize(frames_chosen);
	uint32_t selected = 0;
	std::vector<uint32_t> frame_barrier(histogram_file_header_.frames);
	int32_t start_barrier = 0;
	int32_t end_barrier = 0;
	const int32_t max_barrier_frame = (int)histogram_file_header_.frames - 1;
	uint32_t cur_frame = 0;
	uint32_t start_gap = histogram_file_header_.frames >> 5;
	uint32_t end_gap = histogram_file_header_.frames - start_gap;
	printf("frame barrier: %u of %u\n", start_gap, histogram_file_header_.frames);
	
	for(uint32_t fr = 0; fr < histogram_file_header_.frames; ++fr)
	{		
		cur_frame = frame_scores_[fr].frame;

		if(cur_frame <= start_gap || cur_frame >= end_gap)
			continue;

		if(!frame_barrier[cur_frame])
		{
			
			last_frame = cur_frame;
			frame_selected_[selected].frame = cur_frame;
			frame_selected_[selected].score = frame_scores_[fr].score;

			start_barrier = (int)last_frame - half_gap;
			end_barrier = (int)last_frame + half_gap;

			start_barrier = start_barrier > 0 ? start_barrier : 0;
			end_barrier = end_barrier < max_barrier_frame ? end_barrier : max_barrier_frame;

			memset(&frame_barrier[start_barrier],0xff,sizeof(uint32_t)*(end_barrier-start_barrier+1));
			selected++;

			if(selected >= frames_chosen)
				break;
		}
	}

	frame_ranks_.resize(selected);

	for(uint32_t f = 0; f < selected; ++f)
	{
		frame_ranks_[f].rank = f+1;
		frame_ranks_[f].frame = frame_selected_[f].frame;
	} 

	std::sort(frame_ranks_.begin(),frame_ranks_.end(),sort_by_frame_no);
	save_frame_rank();
}

void RGBHistogram::select_best_frames_thumb(const uint32_t frames_chosen)
{
	if(!frames_chosen)
		return;

	std::sort(frame_scores_.begin(),frame_scores_.end(),sort_by_score);
	uint32_t last_frame = ~0;
	uint32_t frame_distance = 0;
	const uint32_t frame_gap = histogram_file_header_.frames / frames_chosen;
	const int32_t half_gap = (int)frame_gap >> 1;

	frame_selected_thumb_.resize(frames_chosen);
	uint32_t selected = 0;
	std::vector<uint32_t> frame_barrier(histogram_file_header_.frames);
	int32_t start_barrier = 0;
	int32_t end_barrier = 0;
	const int32_t max_barrier_frame = (int)histogram_file_header_.frames - 1;
	uint32_t cur_frame = 0;
	uint32_t start_gap = histogram_file_header_.frames >> 5;
	uint32_t end_gap = histogram_file_header_.frames - start_gap;
	printf("frame barrier thumb: %u of %u\n", start_gap, histogram_file_header_.frames);
	
	for(uint32_t fr = 0; fr < histogram_file_header_.frames; ++fr)
	{		
		cur_frame = frame_scores_[fr].frame;

		if(cur_frame <= start_gap || cur_frame >= end_gap)
			continue;

		if(!frame_barrier[cur_frame])
		{
			
			last_frame = cur_frame;
			frame_selected_thumb_[selected].frame = cur_frame;
			frame_selected_thumb_[selected].score = frame_scores_[fr].score;

			start_barrier = (int)last_frame - half_gap;
			end_barrier = (int)last_frame + half_gap;

			start_barrier = start_barrier > 0 ? start_barrier : 0;
			end_barrier = end_barrier < max_barrier_frame ? end_barrier : max_barrier_frame;

			memset(&frame_barrier[start_barrier],0xff,sizeof(uint32_t)*(end_barrier-start_barrier+1));
			selected++;

			if(selected >= frames_chosen)
				break;
		}
	}

	frame_ranks_thumb_.resize(selected);

	for(uint32_t f = 0; f < selected; ++f)
	{
		frame_ranks_thumb_[f].rank = f+1;
		frame_ranks_thumb_[f].frame = frame_selected_thumb_[f].frame;
	} 

	std::sort(frame_ranks_thumb_.begin(),frame_ranks_thumb_.end(),sort_by_frame_no);
	save_frame_rank(true);
}

bool  RGBHistogram::load_frame_rank(bool thumb)
{
	FILE* frame_rank_file = nullptr;
	uint32_t frames = 0;

	if(!thumb) {
		frame_rank_file = fopen("frame_rank.dat","rb");

		if(frame_rank_file)
		{
			fread(&frames,sizeof(frames),1,frame_rank_file);
			frame_ranks_.resize(frames);
			fread(&frame_ranks_[0],sizeof(Colour::frame_rank),frames,frame_rank_file);
			fclose(frame_rank_file);
			return true;
		}
	}
	else
	{
		frame_rank_file = fopen("frame_rank_thumb.dat","rb");

		if(frame_rank_file)
		{
			fread(&frames,sizeof(frames),1,frame_rank_file);
			frame_ranks_thumb_.resize(frames);
			fread(&frame_ranks_thumb_[0],sizeof(Colour::frame_rank),frames,frame_rank_file);
			fclose(frame_rank_file);
			return true;
		}
	}

	return false;
}

bool RGBHistogram::generate_face_detection_script(std::string model_file)
{
	const size_t sample_frames = frame_ranks_.size();
	char file_name_out[256];

	if(!sample_frames)
		return false;

	std::string face_detect_file = "face_detection.";

#ifdef _WIN32
	face_detect_file.append("bat");
#else
	face_detect_file.append("sh");
#endif

	std::ofstream face_detection(face_detect_file.c_str());

	for(size_t fr = 0; fr < sample_frames; ++fr) {
		sprintf(file_name_out,"dump/frame%05d_%02u.bmp",frame_ranks_[fr].frame,frame_ranks_[fr].rank);
		face_detection << "facedetect " << file_name_out << " " << model_file << std::endl;
	}

	face_detection.close();
	return true;
}

void RGBHistogram::get_image_crop(
	facebox &box, 
	int &width, 
	int &height, 
	std::vector<uint8_t> &crop_image, 
	std::vector<uint8_t> &full_image,
	int &crop_width,
	int &crop_height)
{
	int img_stride = width * 3;
	int width_border = box.width >> 1;
	int height_border = box.height >> 1;

	int x1 = box.x - width_border;
	int x2 = box.x + box.width + width_border;

	int y1 = box.y - width_border;
	int y2 = box.y + box.height + height_border;

	x1 = x1 > 0 ? x1 : 0;
	y1 = y1 > 0 ? y1 : 0;

	x2 = x2 >= width ? (width - 1) : x2;
	y2 = y2 >= height ? (height - 1) : y2;

	crop_width = x2 - x1 + 1;
	crop_height = y2 - y1 + 1;

	crop_image.resize(crop_width*crop_height*3);

	uint8_t* p_image = &full_image[0];
	uint8_t* p_crop_image = &crop_image[0];

	p_image += (x1*3);
	p_image += (y1 * img_stride);
	int crop_stride = crop_width * 3;

	for(int y = y1; y <= y2; ++y)
	{
		memcpy(p_crop_image,p_image,crop_stride);
		p_crop_image += crop_stride;
		p_image += img_stride;
	}
}

bool sort_by_pixels(face_crop &lhs, face_crop &rhs)
{
	return lhs.pixels > rhs.pixels;
}

bool RGBHistogram::extract_face_bitmaps()
{
	const size_t sample_frames = frame_ranks_.size();
	char file_name_out[256];

	if(!sample_frames)
		return false;

	FILE* result_file = NULL;
	std::string face_detection_result_file;
	uint32_t faces = 0;
	std::vector<facebox> boxes;
	std::vector<uint8_t> face_bitmap;
	int width = 0, height = 0, planes = 0;
	std::vector<uint8_t> frame_bitmap;
	uint32_t face_index = 1;
	int crop_width = 0, crop_height = 0;
	std::vector<face_crop> face_crops;
	face_crops.reserve(sample_frames << 2);
	face_crop fcrop;

	for(size_t fr = 0; fr < sample_frames; ++fr) {
		sprintf(file_name_out,"dump/frame%05d_%02u.bmp",frame_ranks_[fr].frame,frame_ranks_[fr].rank);		
		face_detection_result_file = std::string(file_name_out);
		face_detection_result_file.append(".face");

		result_file = NULL;

		result_file = fopen(face_detection_result_file.c_str(),"rb");

		if(result_file)
		{
			fread(&faces,sizeof(uint32_t),1,result_file);
			boxes.resize(faces);
			fread(&boxes[0],sizeof(facebox),faces,result_file);
			fclose(result_file);

			FrameServer::ReadBitmap(std::string(file_name_out),width,height,planes,frame_bitmap);

			for(uint32_t f = 0; f < faces; ++f)
			{
				get_image_crop(boxes[f],width,height,face_bitmap,frame_bitmap,crop_width,crop_height);
				sprintf(file_name_out,"face/%05u_%f.bmp",face_index, boxes[f].score);	
				FrameServer::WriteBitmap(std::string(file_name_out),crop_width,crop_height,3,&face_bitmap[0]);
				fcrop.frame = frame_ranks_[fr].frame;
				fcrop.index = face_index;
				fcrop.pixels = crop_width * crop_height;
				fcrop.width = (unsigned)crop_width;
				fcrop.height = (unsigned)crop_height;
				face_crops.push_back(fcrop);
				face_index++;
			}
		}
	}

	unsigned crop_img_count = (unsigned)face_crops.size();

	FILE* crop_info_file = NULL;

	crop_info_file = fopen("face/face_info.dat","wb");

	if(crop_info_file)
	{
		std::sort(face_crops.begin(),face_crops.end(),sort_by_pixels);
		fwrite(&crop_img_count,sizeof(crop_img_count),1,crop_info_file);
		fwrite(&face_crops[0],sizeof(face_crop),crop_img_count,crop_info_file);
		fclose(crop_info_file);
	}
	return true;
}

bool  RGBHistogram::save_frame_rank(bool thumb)
{
	FILE* frame_rank_file = nullptr;
	

	if(!thumb) {
		frame_rank_file = fopen("frame_rank.dat","wb");
		uint32_t frames = (uint32_t)frame_ranks_.size();

		if(frame_rank_file)
		{
			fwrite(&frames,sizeof(frames),1,frame_rank_file);
			fwrite(&frame_ranks_[0],sizeof(Colour::frame_rank),frames,frame_rank_file);
			fclose(frame_rank_file);
			return true;
		}
	}
	else {
		frame_rank_file = fopen("frame_rank_thumb.dat","wb");
		uint32_t frames = (uint32_t)frame_ranks_thumb_.size();

		if(frame_rank_file)
		{
			fwrite(&frames,sizeof(frames),1,frame_rank_file);
			fwrite(&frame_ranks_thumb_[0],sizeof(Colour::frame_rank),frames,frame_rank_file);
			fclose(frame_rank_file);
			return true;
		}
	}

	return false;
}

bool RGBHistogram::score(uint8_t* rgb_ptr, uint32_t count)
{
	Colour::rgba* p_rgba = (Colour::rgba*)rgb_ptr;
	Colour::rgba colour;
	colour.a = 0;
	uint32_t * p_colour;
	p_colour = (uint32_t *)&colour;
	bool found_colour = false;

	uint8_t* rgb_ptr1 = rgb_ptr + (score_width_ * 3);
	Colour::rgba* p_rgba_1 = (Colour::rgba*)rgb_ptr1;
	uint64_t colour_grad = 0ULL;
	uint32_t diff_red = 0;
	uint32_t diff_green = 0;
	uint32_t diff_blue = 0;
	uint32_t intensity = 0;

	double log_prob = 0.0;

	for(uint32_t y = 0; y < score_height_; ++y)
	{
		for(uint32_t x = 0; x < score_width_; ++x)
		{
			colour.r = p_rgba->r;
			colour.g = p_rgba->g;
			colour.b = p_rgba->b;
			diff_red = (int)p_rgba->r - (int)p_rgba_1->r;
			diff_green = (int)p_rgba->g - (int)p_rgba_1->g;
			diff_blue = (int)p_rgba->b - (int)p_rgba_1->b;

			diff_red *= diff_red;
			diff_green *= diff_green;
			diff_blue *= diff_blue;

			intensity += (uint32_t)colour.r;
			intensity += (uint32_t)colour.g;
			intensity += (uint32_t)colour.b;

			colour_grad += (diff_red + diff_green + diff_blue);
			// right
			rgb_ptr += 3;
			p_rgba = (Colour::rgba*)rgb_ptr;

			diff_red = (int)p_rgba->r - (int)colour.r;
			diff_green = (int)p_rgba->g - (int)colour.g;
			diff_blue = (int)p_rgba->b - (int)colour.b;

			diff_red *= diff_red;
			diff_green *= diff_green;
			diff_blue *= diff_blue;

			colour_grad += (diff_red + diff_green + diff_blue);

			rgb_ptr1 += 3;
			p_rgba_1 = (Colour::rgba*)rgb_ptr1;

			log_prob += log_probability_[*p_colour];
		}

		rgb_ptr += 3;
		p_rgba = (Colour::rgba*)rgb_ptr;
		rgb_ptr1 += 3;
		p_rgba_1 = (Colour::rgba*)rgb_ptr1;
	}

	intensity /= (score_height_*score_width_*3);

	if(intensity < 50)
	{
		colour_grad = 0ULL;
	}

	frame_scores_[frame_no_].frame = frame_no_;
	frame_scores_[frame_no_].score = log_prob + (double)colour_grad;

	frame_no_++;
	return found_colour;
}


bool RGBHistogram::update(uint8_t* rgb_ptr, uint32_t count)
{
	Colour::rgba* p_rgba = (Colour::rgba*)rgb_ptr;
	Colour::rgba colour;
	colour.a = 0;
	uint32_t * p_colour;
	p_colour = (uint32_t *)&colour;
	bool found_colour = false;

	histogram_file_header_.frames++;
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
