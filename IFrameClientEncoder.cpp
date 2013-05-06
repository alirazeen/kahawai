#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClientEncoder.h"
#include "X264Encoder.h"

IFrameClientEncoder::IFrameClientEncoder(void)
	: _encoder(NULL)
{
}


IFrameClientEncoder::~IFrameClientEncoder(void)
{
	if (_encoder != NULL)
		delete _encoder;
}

bool IFrameClientEncoder::Initialize(ConfigReader* configReader, IFrameClientMuxer* muxer)
{
	// Read all encoder settings
	_height = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_HEIGHT);
	_width = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_WIDTH);
	_crf = configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_CRF);
	_preset = configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_ENCODER_LEVEL);
	_gop = configReader->ReadIntegerValue(CONFIG_IFRAME,CONFIG_GOP_SIZE);

	//Initialize encoder
	_encoder = new X264Encoder(_height, _width, _fps, _crf, _preset, _gop);
	_muxer = muxer;

	return (_encoder != NULL);
}

bool IFrameClientEncoder::ReceiveTransformedPicture(x264_picture_t* transformPicture)
{
	bool result = Encode(transformPicture);
	return result;
}

int IFrameClientEncoder::Encode(x264_picture_t* transformPicture)
{
	void* compressedFrame = NULL;
	
	//We only want to encode an iframe here
	//So set the appropriate x264 encoder settings
	transformPicture->i_type=X264_TYPE_IDR;
	transformPicture->i_qpplus1 = 1;

	int size = _encoder->Encode(transformPicture, &compressedFrame);
	_muxer->ReceiveIFrame(compressedFrame, size);

	return (size > 0);
}

#endif
