#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClientMuxer.h"


IFrameClientMuxer::IFrameClientMuxer(void)
{
}


IFrameClientMuxer::~IFrameClientMuxer(void)
{
}

bool IFrameClientMuxer::Initialize(ConfigReader* configReader)
{

	_serverPort = configReader->ReadIntegerValue(CONFIG_SERVER,CONFIG_SERVER_PORT);
	configReader->ReadProperty(CONFIG_SERVER,CONFIG_SERVER_ADDRESS, _serverIP);

	
	return true;
}

bool IFrameClientMuxer::InitSocketToServer()
{
	// TODO: Connect to server
	return true;
}


bool IFrameClientMuxer::Decode()
{
	// TODO: Implement this
	return false;
}

bool IFrameClientMuxer::Show()
{
	// TODO: Implement this
	return false;
}



#endif