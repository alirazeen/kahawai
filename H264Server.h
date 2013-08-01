#pragma once
#include "kahawaiserver.h"
class H264Server :
	public KahawaiServer
{
public:
	H264Server(void);
	~H264Server(void);

protected:
	//Kahawai Lifecycle

	bool	StartOffload();
	void	OffloadAsync();
	bool	Initialize();

	int		Encode(void** compressedFrame);
	bool	Send(void** compressedFrame, int frameSize);

	//Input Handling
	void*	HandleInput();
	int		GetFirstInputFrame();


private:
	bool				_inputConnectionDone;
	CRITICAL_SECTION	_inputSocketCS;
	CONDITION_VARIABLE	_inputSocketCV;
};

