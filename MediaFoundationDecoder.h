#pragma once
#include "videodecoder.h"
class MediaFoundationDecoder :
	public VideoDecoder
{
public:
	MediaFoundationDecoder(void);
	~MediaFoundationDecoder(void);
	bool Decode(kahawaiTransform apply = 0, byte* patch = 0);
	bool Show();
};

