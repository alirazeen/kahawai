#pragma once
#include "kahawaiserver.h"
class IFrameServer :
	public KahawaiServer
{
public:

	IFrameServer(void);
	~IFrameServer(void);

	bool StartOffload();
protected:

	bool	Initialize();

	//Kahawai life-cycle
	bool	Capture(int width, int height);
	
	void	OffloadAsync();
	bool	Transform(int width, int height);
	int		Encode(void** compressedFrame);
	bool	Send(void** compressedFrame, int frameSize);

	//Input Handling (NO OP)
	void*	HandleInput();
	int		GetFirstInputFrame();


	int		_gop;
	int		_currFrameNum;

private:

	bool				_inputConnectionDone;
	CRITICAL_SECTION	_inputSocketCS;
	CONDITION_VARIABLE	_inputSocketCV;
};

