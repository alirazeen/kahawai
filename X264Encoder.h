#pragma once
#include "videoencoder.h"
#include "kahawaiBase.h"

class X264Encoder :
	public VideoEncoder
{
public:
	X264Encoder(int height, int width, int fps, int crf, int preset, int gop=-1);
	~X264Encoder(void);
	int Encode(void* pictureIn, void** pictureOut, kahawaiTransform apply = 0, byte* base = 0); 
	int GetBlackFrame(SwsContext* convertCtx, void** pictureOut);

protected:
	x264_t* _encoder;
	int _height;
	int _width;
	int _y420pFrameSize; //The size of the frame in YUV420p format
	int _fps;
	int _gop; 
	int _crf;
	int _preset;
};

