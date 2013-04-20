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
	if (_iFrame != NULL)
	{
		delete _iFrame;
		_iFrame = NULL;
	}

	if (_pFrame != NULL)
	{
		delete _pFrame;
		_pFrame = NULL;
	}
}

bool IFrameClientMuxer::Initialize(ConfigReader* configReader)
{

	_serverPort = configReader->ReadIntegerValue(CONFIG_SERVER,CONFIG_SERVER_PORT);
	_localMuxerPort = _serverPort + PORT_OFFSET_IFRAME_MUXER;
	configReader->ReadProperty(CONFIG_SERVER,CONFIG_SERVER_ADDRESS, _serverIP);

	_gop = configReader->ReadIntegerValue(CONFIG_IFRAME,CONFIG_GOP_SIZE);
	_currFrameNum = 0;

	//Initialize Synchronization
	InitializeCriticalSection(&_sendingFrameCS);
	InitializeConditionVariable(&_iframeWaitingCV);
	InitializeConditionVariable(&_pframeWaitingCV);

	//Initialize the buffers where we store the received I/P frames
	int width = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_WIDTH);
	int height = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_HEIGHT);
	_iFrame = new byte[height * width * 2]; // twice the size of a normal frame
	_pFrame = new byte[height * width * _gop * 2]; // twice the size of the GOP 

	return true;
}

bool IFrameClientMuxer::BeginOffload()
{
	if (!InitSocketToServer())
	{
		KahawaiLog("IFrameClientMuxer::InitSocketToServer() failed", KahawaiError);
		return false;
	}

	if (!InitLocalSocket()) {
		KahawaiLog("IFrameClientMuxer::InitLocalSocket() failed", KahawaiError);
		return false;
	}
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
	
	return _socketToDecoder != INVALID_SOCKET;
}

bool IFrameClientMuxer::ReceiveIFrame(void** compressedFrame, int size)
{
	EnterCriticalSection(&_sendingFrameCS);
	{
		while(_currFrameNum % _gop != 0)
		{
			SleepConditionVariableCS(&_iframeWaitingCV, &_sendingFrameCS, INFINITE);	
			//TODO: Should return false and exit loop if we not offloading anymore
		}

		SendFrameToLocalSocket((char*)(*compressedFrame), size);
	}
	WakeConditionVariable(&_pframeWaitingCV);
	LeaveCriticalSection(&_sendingFrameCS);
	return true;
}


DWORD WINAPI IFrameClientMuxer::AsyncReceivePFrames(void* Param)
{
	IFrameClientMuxer* This = (IFrameClientMuxer*) Param;
	This->ReceivePFrames();

	return 0;
}

void IFrameClientMuxer::ReceivePFrames()
{
	EnterCriticalSection(&_sendingFrameCS);
	{
		while(_currFrameNum%_gop == 0)
		{
			SleepConditionVariableCS(&_pframeWaitingCV, &_sendingFrameCS, INFINITE);
			//TODO: Should return and exit loop if we are not offloading anymore
		}

		//Receive P-frame size
		int length=0;
		if (recv(_socketToServer, (char*)&length,sizeof(int), 0) == SOCKET_ERROR)
		{
			char errorMsg[100];
			int errorCode = WSAGetLastError();
			sprintf_s(errorMsg,"Unable to receive P frame size. Error code: %d",errorCode);
			KahawaiLog(errorMsg, KahawaiError);
			return;
		}

		//Receive P frame completely
		int receivedBytes = 0;
		while(receivedBytes < length)
		{
			int burst = 0;
			burst = recv(_socketToServer, (char*)_pFrame+receivedBytes, length, 0);
			VERIFY(burst != SOCKET_ERROR);
			receivedBytes += burst;
		}

		//Send out P frame
		SendFrameToLocalSocket((char*)_pFrame, length);
	}
	WakeConditionVariable(&_iframeWaitingCV);
	LeaveCriticalSection(&_sendingFrameCS);
}

bool IFrameClientMuxer::SendFrameToLocalSocket(char* frame, int size)
{
	int sent = 0;
	while (sent < size)
	{
		sent += send(_socketToDecoder, frame+sent, size, 0);
	}

	_currFrameNum++;
	return true;
}

#endif