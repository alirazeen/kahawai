#pragma once
#include "kahawaiclient.h"
#include "IFrameClientEncoder.h"
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
	void		OffloadAsync();
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
	int			_currFrameNum;


	bool		SendTransformPictureEncoder();

	static DWORD WINAPI		AsyncDecodeShow(void* Param);
	void					DecodeShow();
};

