#pragma once
#include "kahawaiBase.h"


class VideoDecoder
{
public:
	virtual ~VideoDecoder(void) {}

	//appy and patch used when decoding with deltas. 
	virtual bool Decode(transform apply = 0, byte* patch = 0) = 0;
	virtual bool Show() = 0;

protected:
	int _width;
	int _height;
};

