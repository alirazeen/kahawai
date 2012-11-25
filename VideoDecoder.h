#pragma once
#include "kahawaiBase.h"


class VideoDecoder
{
public:
	virtual ~VideoDecoder(void) {}

	//appy and patch used when decoding with deltas. 
	virtual bool Decode(kahawaiTransform apply = 0, byte* patch = 0) = 0;
	virtual bool Show() = 0;


	//Status of the decoded stream
	//Returns the numbers of frames displayed
	int GetDisplayedFrames() { return _displayedFrames; }
	void EnableFrameLogging(bool enabled) { _saveFrames = enabled;}

protected:
	int _width;
	int _height;
	
	int _displayedFrames;

	//Debug
	bool _saveFrames;

	VideoDecoder() { _displayedFrames = 0; }

};

