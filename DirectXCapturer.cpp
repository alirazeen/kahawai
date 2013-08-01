#include<iostream> //Include just so VStudio doesn't complain (when compiling in "original" mode)
#ifdef KAHAWAI
#include "DirectXCapturer.h"
#include <d3d9.h>

DirectXCapturer::DirectXCapturer(int width, int height)
	:_height(height), _width(width)
{
	_buffer = new uint8_t[width*height*SOURCE_BITS_PER_PIXEL];
}


DirectXCapturer::~DirectXCapturer(void)
{
	delete[] _buffer;
}

uint8_t* DirectXCapturer::CaptureScreen(void* args)
{
	uint8_t* bytes = (uint8_t*)args;
	KahawaiLog("About to return from capture screen", KahawaiError);
	return bytes;
	//return _buffer;
}
#endif