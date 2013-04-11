#pragma once
#include "kahawaiserver.h"
class IFrameClientEncoder
{
public:
	IFrameClientEncoder(void);
	~IFrameClientEncoder(void);


	bool				Initialize(ConfigReader* configReader);

	int					Encode(void** transformedFrame);
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

