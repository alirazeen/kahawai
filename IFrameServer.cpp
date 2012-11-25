#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameServer.h"


IFrameServer::IFrameServer(void)
{
}


IFrameServer::~IFrameServer(void)
{
}



int IFrameServer::Encode(void** compressedFrame)
{
	return true;
}

bool IFrameServer::Send(void** compressedFrame, int frameSize)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
//Input Handling
//////////////////////////////////////////////////////////////////////////
void* IFrameServer::HandleInput(void* input)
{
	throw 1;
	return input;
}


int IFrameServer::GetFirstInputFrame()
{
	throw 1;
	return 0;
}


#endif