#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClientMuxer.h"


IFrameClientMuxer::IFrameClientMuxer(void)
	:_socketToServer(INVALID_SOCKET)
{
}


IFrameClientMuxer::~IFrameClientMuxer(void)
{
}

bool IFrameClientMuxer::Initialize(ConfigReader* configReader)
{

	_serverPort = configReader->ReadIntegerValue(CONFIG_SERVER,CONFIG_SERVER_PORT);
	configReader->ReadProperty(CONFIG_SERVER,CONFIG_SERVER_ADDRESS, _serverIP);

	if (!InitSocketToServer())
	{
		KahawaiLog("IFrameClientMuxer::InitSocketToServer() failed", KahawaiError);
		return false;
	}

	if (!InitLocalSocket()) {
		KahawaiLog("IFrameClientMuxer::InitLocalSocket() failed", KahawaiError);
		return false;
	}

	return true;
}

bool IFrameClientMuxer::InitSocketToServer()
{
	//Connect to server
	int attempts = 0;
	while(_socketToServer == INVALID_SOCKET && attempts < MAX_ATTEMPTS)
	{
		_socketToServer = CreateSocketToServer(_serverIP, _serverPort);
#ifdef WIN32
		Sleep(5000);
#endif
	}

	//Spawn thread to get the P frames
	if (_socketToServer != INVALID_SOCKET)
		bool threadCreated = CreateKahawaiThread(AsyncReceivePFrames, this);

	return _socketToServer != INVALID_SOCKET;
}

bool IFrameClientMuxer::InitLocalSocket()
{
	// TODO: Listen to incoming connections
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


DWORD WINAPI IFrameClientMuxer::AsyncReceivePFrames(void* Param)
{
	IFrameClientMuxer* This = (IFrameClientMuxer*) Param;
	This->ReceivePFrames();

	return 0;
}

void IFrameClientMuxer::ReceivePFrames()
{
	// TODO: Receive P frames from server
}

#endif