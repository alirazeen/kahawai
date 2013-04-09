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

	bool		ShouldSkip();

protected:
	//Kahawai life-cycle
	bool		Decode();
	bool		Show();

	//Input Handling Profile
	int			GetFirstInputFrame();

	//Components
	IFrameClientDecoder* _decoderComponent;
	IFrameClientEncoder* _encoderComponent;



};

