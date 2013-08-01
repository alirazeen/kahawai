#pragma once
#include "Capturer.h"


class DirectXCapturer : 
	public Capturer
{
public:
	DirectXCapturer(int width, int height);
	~DirectXCapturer(void);
	uint8_t* CaptureScreen(void* args);

private:
	int _width;
	int _height;
	uint8_t* _buffer;
};

