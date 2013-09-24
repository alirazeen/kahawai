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
	void*		HandleInput();

	bool		StartOffload();

protected:
	int			_gop;

	bool		IsHD();

	//Kahawai life-cycle. We cannot use the lifecycle code that is in 
	//KahawaiClient and we will need to reimplement OffloadAsync. This is 
	//because of an issue with the FFMpegDecoder that can cause a deadlock.
	//During the game startup, the decoder needs both frames N and N+1 before
	//it can display a frame N. So it runs into a situation where it needs the
	//I-frame in the next GOP before it can display the last P frame in the current
	//GOP. If it doesn't receive the next I-frame, it will just block.
	//
	//However, the next I-frame is only provided by the Transform() method. So if 
	//Decode() blocks, there is no way Transform can be reached and we reach a deadlock
	//
	//The current solution is to run Transform in its own thread and to run DecodeShow
	//in a different thread
	void		OffloadAsync();
	bool		Capture(int width, int height, void* args);
	bool		Transform(int width, int height, int frameNum);
	bool		Decode();
	bool		Show();
	void		GrabInput();

	//Input Handling Profile
	int			GetFirstInputFrame();

	//Components
	IFrameClientEncoder*	_encoderComponent;
	IFrameClientMuxer*		_muxerComponent;

private:

	//Need a separate counter for the number of transformed frames
	//since Transform and Decode/Show run in different threads 
	int			_numTransformedFrames; //Number of frames transformed so far

	//Input-handling variables
	void*				_lastCommand;

	CRITICAL_SECTION	_inputCS;
	CONDITION_VARIABLE	_inputQueueEmptyCV;

	bool				_inputConnectionDone;
	CRITICAL_SECTION	_inputSocketCS;
	CONDITION_VARIABLE	_inputSocketCV;

	bool		SendTransformPictureEncoder();

	//We will run DecodeShow in a different thread due to reasons explained above.
	static DWORD WINAPI		AsyncDecodeShow(void* Param);
	void					DecodeShow();

	int		_numInputSampled;
	int		_numInputProcessed;
};

