#pragma once
#include "kahawaiclient.h"
#include "IframeClientEncoder.h"
#include "IFrameClientMuxer.h"

class IFrameClient :
	public KahawaiClient
{
public:
	IFrameClient(void);
	~IFrameClient(void);

	bool		Initialize();
	bool		InitializeDecoder();
	bool		ShouldSkip();
	void*		HandleInput(void*);

protected:
	int			_gop;

	//Kahawai life-cycle
	bool		Transform(int width, int height);
	bool		Decode();
	bool		Show();

	//Input Handling Profile
	int			GetFirstInputFrame();

	//Components
	IFrameClientEncoder*	_encoderComponent;
	IFrameClientMuxer*		_muxerComponent;

private:
	//Input-handling variables
	void*		_lastCommand;


	bool		SendTransformPictureoEncoder();
};

