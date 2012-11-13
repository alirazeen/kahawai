#pragma once
#include "Capturer.h"


class DirectXCapturer : 
	public Capturer
{
public:
	DirectXCapturer(void);
	~DirectXCapturer(void);
	uint8_t* CaptureScreen();

};

