#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClientMuxer.h"


IFrameClientMuxer::IFrameClientMuxer(void)
	:_socketToServer(INVALID_SOCKET),
	_socketToDecoder(INVALID_SOCKET),
	_receivedIFrame(FALSE),
	_iFrameSize(0),
	_receivedPFrame(FALSE),
	_pFrameSize(0)
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

	InitializeCriticalSection(&_initSocketCS);
	InitializeConditionVariable(&_initSocketCV);

	InitializeCriticalSection(&_receiveIFrameCS);
	InitializeConditionVariable(&_receivingIFrameCV);
	InitializeConditionVariable(&_iFrameConsumedCV);

	InitializeCriticalSection(&_receivePFrameCS);
	InitializeConditionVariable(&_receivingPFrameCV);
	InitializeConditionVariable(&_pFrameConsumedCV);

	//Initialize the buffers where we store the received I/P frames
	int width = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_WIDTH);
	int height = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_HEIGHT);
	_iFrame = new byte[height * width * DEFAULT_BIT_DEPTH]; // size of a normal frame
	_pFrame = new byte[height * width * DEFAULT_BIT_DEPTH * _gop]; // size of the GOP

	return true;
}

bool IFrameClientMuxer::BeginOffload()
{
	
	//Establish a connection to the remote server
	if (!InitSocketToServer())
	{
		KahawaiLog("IFrameClientMuxer::InitSocketToServer() failed", KahawaiError);
		return false;
	}

	//Establish a connection to the local decoder
	bool threadCreated = CreateKahawaiThread(AsyncInitLocalSocket, this);

	if (threadCreated)
		//Create the thread that will send frames to the local decoder
		threadCreated = CreateKahawaiThread(AsyncSendFrames, this);

	return threadCreated;
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

DWORD WINAPI IFrameClientMuxer::AsyncInitLocalSocket(void* Param)
{
	IFrameClientMuxer* This = (IFrameClientMuxer*)Param;
	This->InitLocalSocket();
	return 0;
}

bool IFrameClientMuxer::InitLocalSocket()
{
	EnterCriticalSection(&_initSocketCS);
	{
		_socketToDecoder = CreateSocketToClient(_localMuxerPort);
	}
	WakeConditionVariable(&_initSocketCV);
	LeaveCriticalSection(&_initSocketCS);

	return _socketToDecoder != INVALID_SOCKET;
}

DWORD WINAPI IFrameClientMuxer::AsyncReceivePFrames(void* Param)
{
	IFrameClientMuxer* This = (IFrameClientMuxer*) Param;

	//TODO: Need to exit loop at _some_ point
	while(true)
	{
		This->ReceivePFrame();
	}

	return 0;
}

DWORD WINAPI IFrameClientMuxer::AsyncSendFrames(void* Param)
{
	IFrameClientMuxer* This = (IFrameClientMuxer*) Param;
	This->SendFrames();
	
	return 0;
}

void IFrameClientMuxer::SendFrames()
{

	//Wait for the decoder socket to be created
	EnterCriticalSection(&_initSocketCS);
	{
		while(_socketToDecoder == INVALID_SOCKET)
			SleepConditionVariableCS(&_initSocketCV, &_initSocketCS, INFINITE);
	}
	LeaveCriticalSection(&_initSocketCS);

	//TODO: There should be some termination condition where we stop
	//this loop
	while(true)
	{
		
		if (_currFrameNum % _gop == 0)
		{
			EnterCriticalSection(&_receiveIFrameCS);
			{
				while(!_receivedIFrame)
					SleepConditionVariableCS(&_receivingIFrameCV, &_receiveIFrameCS, INFINITE);
			
				//Send it out
				SendFrameToLocalDecoder((char*)_iFrame, _iFrameSize);

				//We have consumed the frame
				_receivedIFrame = false;
			}
			WakeConditionVariable(&_iFrameConsumedCV);
			LeaveCriticalSection(&_receiveIFrameCS);
		} else 
		{
			EnterCriticalSection(&_receivePFrameCS);
			{
				while (!_receivedPFrame)
					SleepConditionVariableCS(&_receivingPFrameCV, &_receivePFrameCS, INFINITE);

				//Send it out
				SendFrameToLocalDecoder((char*)_pFrame, _pFrameSize);

				//We have consumed the frame
				_receivedPFrame = false;
			}
			WakeConditionVariable(&_pFrameConsumedCV);
			LeaveCriticalSection(&_receivePFrameCS);
		}

	}

}

bool IFrameClientMuxer::SendFrameToLocalDecoder(char* frame, int size)
{
	int sent = 0;
	while (sent < size)
	{
		sent += send(_socketToDecoder, frame+sent, size, 0);
	}

	_currFrameNum++;
	return true;
}

bool IFrameClientMuxer::ReceiveIFrame(void* compressedFrame, int size)
{	
	EnterCriticalSection(&_receiveIFrameCS);
	{
		while(_receivedIFrame)
			SleepConditionVariableCS(&_iFrameConsumedCV, &_receiveIFrameCS, INFINITE);

		memcpy(_iFrame,compressedFrame,size);
		_receivedIFrame = true;
		_iFrameSize = size;
	}
	WakeConditionVariable(&_receivingIFrameCV);
	LeaveCriticalSection(&_receiveIFrameCS);
	return true;
}

void IFrameClientMuxer::ReceivePFrame()
{
	EnterCriticalSection(&_receivePFrameCS);
	{
		while(_receivedPFrame) 
			SleepConditionVariableCS(&_pFrameConsumedCV, &_receivePFrameCS, INFINITE);

		//Receive P-frame size
		int length=0;
		if (recv(_socketToServer, (char*)&length,sizeof(int), 0) == SOCKET_ERROR)
		{
			char errorMsg[100];
			int errorCode = WSAGetLastError();
			sprintf_s(errorMsg,"Unable to receive P frame size. Error code: %d",errorCode);
			KahawaiLog(errorMsg, KahawaiError);

			LeaveCriticalSection(&_receivePFrameCS);
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

		_receivedPFrame = true;
		_pFrameSize = length;
		
	}
	WakeConditionVariable(&_receivingPFrameCV);
	LeaveCriticalSection(&_receivePFrameCS);
}

#endif