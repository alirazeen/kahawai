#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IframeClientEncoder.h"
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

bool IFrameClientEncoder::Initialize(ConfigReader* configReader)
{
	// Read all encoder settings
	_height = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_HEIGHT);
	_width = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_WIDTH);
	_crf = configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_CRF);
	_preset = configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_ENCODER_LEVEL);
	_gop = configReader->ReadIntegerValue(CONFIG_IFRAME,CONFIG_GOP_SIZE);

	//Initialize encoder
	_encoder = new X264Encoder(_height, _width, _fps, _crf, _preset, _gop);

	return (_encoder != NULL);
}

int IFrameClientEncoder::Encode(void** transformedFrame)
{
	// TODO: Implement this
	return -1;
}

bool IFrameClientEncoder::Send(void** compressedFrame, int frameSize)
{
	// TODO: Implement this
	return false;
}

#endif
