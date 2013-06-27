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
	bool	Capture(int width, int height, void* args);
	
	void	OffloadAsync();
	bool	Transform(int width, int height);
	int		Encode(void** compressedFrame);
	bool	Send(void** compressedFrame, int frameSize);

	//Input Handling (NO OP)
	void*	HandleInput();
	int		GetFirstInputFrame();

	bool isClient();
	bool isSlave();
	bool isMaster();

	int		_gop;
	int		_currFrameNum;

private:

	bool				_connectionAttemptDone;
	CRITICAL_SECTION	_socketCS;
	CONDITION_VARIABLE	_socketCV;
};

