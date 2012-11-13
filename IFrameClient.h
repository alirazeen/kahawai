#pragma once
#include "kahawaiclient.h"
#include "IframeClientEncoder.h"
#include "IframeClientDecoder.h"

class IFrameClient :
	public KahawaiClient
{
public:
	IFrameClient(void);
	~IFrameClient(void);

protected:
	//Kahawai life-cycle
	bool		Decode();
	bool		Show();

	//Components
	IFrameClientDecoder* _decoderComponent;
	IFrameClientEncoder* _encoderComponent;

};

