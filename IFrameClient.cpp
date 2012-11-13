#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClient.h"


IFrameClient::IFrameClient(void)
{
}


IFrameClient::~IFrameClient(void)
{
}


bool IFrameClient::Decode()
{
	KahawaiLog("Not implemented: IframeClient::Decode", KahawaiError);
	return false;
}

bool IFrameClient::Show()
{
	KahawaiLog("Not implemented: IFrameClient::Show", KahawaiError);
	return false;
}
#endif
