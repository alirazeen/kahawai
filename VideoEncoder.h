#pragma once
#include "kahawaiBase.h"
class VideoEncoder
{
public:
	virtual ~VideoEncoder(void) {}

	/**
	 * General interface for encoding video, regardless of implementation 
	 * @param pictureIn the original source
	 * @param pictureOut the encoded result
	 * @return the size in bytes of pictureOut. Zero or less on error
	 */
	virtual int Encode(void* pictureIn, void** pictureOut, transform apply = 0, byte* base = 0) = 0; 
};

