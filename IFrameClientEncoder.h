#pragma once
#include "kahawaiserver.h"
#include "IFrameClientMuxer.h"

class Measurement;
class IFrameClientEncoder
{
public:
	IFrameClientEncoder(void);
	~IFrameClientEncoder(void);


	bool				Initialize(ConfigReader* configReader, IFrameClientMuxer* muxer);
	bool				ReceiveTransformedPicture(x264_picture_t* transformPicture);
	int					Encode(x264_picture_t* transformPicture);
	void				SetMeasurement(Measurement* _measurement);	

private:

	// Encoder settings
	int					_height;
	int					_width;
	static const int	_fps = TARGET_FPS;
	int					_crf;
	int					_preset;
	int					_gop;

	VideoEncoder*		_encoder;
	IFrameClientMuxer*	_muxer;
};

