#pragma once
#include "kahawaiserver.h"
class IFrameServer :
	public KahawaiServer
{
public:

	IFrameServer(void);
	~IFrameServer(void);

protected:

	bool	Initialize();

	//Kahawai life-cycle
	void	OffloadAsync();
	int		Encode(void** compressedFrame);
	bool	Send(void** compressedFrame, int frameSize);

	//Input Handling (NO OP)
	void*	HandleInput(void* input);
	int		GetFirstInputFrame();


	int		_gop;

};

