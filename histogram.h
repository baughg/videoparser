#ifndef HISTOGRAM_H_
#define HISTOGRAM_H_
#include <vector>
#include <stdint.h>

typedef struct colour_frequency{
	colour_frequency()
	{
		colour = 0;
		freq = 0;
	}
	uint32_t colour;
	uint64_t freq;
}colour_frequency;

typedef struct colour_group {
	colour_group()
	{
		group = 0;
	}

	colour_frequency col_freq;
	uint32_t group;
}colour_group;

class Histogram
{
public:
	Histogram(void);
	~Histogram(void);
protected:
	virtual void init() = 0;
	std::vector<uint64_t> hist_;
	std::vector<colour_frequency> colour_frequency_;
	std::vector<double> log_probability_;
	std::vector<colour_group> colour_group_;
};
#endif

