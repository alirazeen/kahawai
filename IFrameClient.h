#pragma once
#include "kahawaiclient.h"
#include "IframeClientEncoder.h"

class IFrameClient :
	public KahawaiClient
{
public:
	IFrameClient(void);
	~IFrameClient(void);

	bool		ShouldSkip();
	void*		HandleInput(void*);

protected:
	//Kahawai life-cycle
	bool		Decode();
	bool		Show();

	//Input Handling Profile
	int			GetFirstInputFrame();

	//Components
	IFrameClientEncoder* _encoderComponent;



};

