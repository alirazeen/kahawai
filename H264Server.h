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
	void	OffloadAsync();
	bool	Initialize();
	int		Encode(void** compressedFrame);
	bool	Send(void** compressedFrame, int frameSize);

	//Input Handling
	void*	HandleInput();
	int		GetFirstInputFrame();


};

