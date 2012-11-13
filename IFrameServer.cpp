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

#endif