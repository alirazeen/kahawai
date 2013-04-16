#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClientMuxer.h"


IFrameClientMuxer::IFrameClientMuxer(void)
	:_socketToServer(INVALID_SOCKET),
	_socketToDecoder(INVALID_SOCKET)
{
}


IFrameClientMuxer::~IFrameClientMuxer(void)
{
}

bool IFrameClientMuxer::Initialize(ConfigReader* configReader)
{

	_serverPort = configReader->ReadIntegerValue(CONFIG_SERVER,CONFIG_SERVER_PORT);
	_localMuxerPort = _serverPort + PORT_OFFSET_IFRAME_MUXER;
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
	bool threadCreated = false;
	if (_socketToServer != INVALID_SOCKET)
		threadCreated = CreateKahawaiThread(AsyncReceivePFrames, this);

	return _socketToServer != INVALID_SOCKET && threadCreated == true;
}

bool IFrameClientMuxer::InitLocalSocket()
{
	_socketToDecoder = CreateSocketToClient(_localMuxerPort);
	
	bool threadCreated = false;
	if (_socketToDecoder != INVALID_SOCKET)
		threadCreated = CreateKahawaiThread(AsyncSendMuxedFrames, this);

	return _socketToDecoder != INVALID_SOCKET && threadCreated == true;
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


DWORD WINAPI IFrameClientMuxer::AsyncSendMuxedFrames(void* Param)
{
	IFrameClientMuxer* This = (IFrameClientMuxer*) Param;
	This->SendMuxedFrames();

	return 0;
}

void IFrameClientMuxer::SendMuxedFrames()
{
	// TODO: Send muxed frames to decoder
}

#endif