#include<iostream> //Include just so VStudio doesn't complain (when compiling in "original" mode)
#ifdef KAHAWAI
#include "DirectXCapturer.h"

DirectXCapturer::DirectXCapturer(void)
{
}


DirectXCapturer::~DirectXCapturer(void)
{
}

uint8_t* DirectXCapturer::CaptureScreen()
{
	KahawaiLog("DirectX Capturer Not implemented", KahawaiError);
	return 0;
}
#endif