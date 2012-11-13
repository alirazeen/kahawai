#pragma once
#include "capturer.h"

class OpenGLCapturer :
	public Capturer
{
public:
	OpenGLCapturer(int width, int height);
	~OpenGLCapturer(void);

	uint8_t* CaptureScreen();
private:
	int _width;
	int _height;
	uint8_t* _buffer;
};

