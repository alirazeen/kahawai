#pragma once
#include "kahawaiserver.h"
class IFrameClientEncoder
{
public:
	IFrameClientEncoder(void);
	~IFrameClientEncoder(void);


	bool				Initialize(ConfigReader* configReader);
	bool				ReceiveTransformedPicture(x264_picture_t* transformPicture);
	int					Encode(x264_picture_t* transformPicture);
	bool				Send(void** compressedFrame, int frameSize);

private:

	// Encoder settings
	int					_height;
	int					_width;
	static const int	_fps = TARGET_FPS;
	int					_crf;
	int					_preset;
	int					_gop;

	VideoEncoder*		_encoder;
};

