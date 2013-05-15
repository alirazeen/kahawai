#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClientMuxer.h"

// Number of buffers to create in the circular buffer
#define NUM_BUFFERS 10

IFrameClientMuxer::IFrameClientMuxer(void)
	:_socketToServer(INVALID_SOCKET),
	_socketToDecoder(INVALID_SOCKET),
	_iFrameBuffers(NULL),
	_iFrameMaxSize(0),
	_pFrameBuffers(NULL),
	_pFrameMaxSize(0)
{
}


IFrameClientMuxer::~IFrameClientMuxer(void)
{

	if (_iFrameBuffers != NULL) 
	{
		delete _iFrameBuffers;
		_iFrameBuffers = NULL;
	}

	if (_pFrameBuffers != NULL)
	{
		delete _pFrameBuffers;
		_pFrameBuffers = NULL;
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

	InitializeCriticalSection(&_iFrameCS);
	InitializeConditionVariable(&_iFrameWaitForFrameCV);
	InitializeConditionVariable(&_iFrameWaitForSpaceCV);

	InitializeCriticalSection(&_pFrameCS);
	InitializeConditionVariable(&_pFrameWaitForFrameCV);
	InitializeConditionVariable(&_pFrameWaitForSpaceCV);

	//Initialize the buffers where we store the received I/P frames
	int width = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_WIDTH);
	int height = configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_HEIGHT);

	_iFrameMaxSize = height * width * DEFAULT_BIT_DEPTH;
	_pFrameMaxSize = height * width * DEFAULT_BIT_DEPTH * _gop;

	_iFrameBuffers = new CircularBuffer(_iFrameMaxSize, NUM_BUFFERS);
	_pFrameBuffers = new CircularBuffer(_pFrameMaxSize, NUM_BUFFERS);

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
		threadCreated = CreateKahawaiThread(AsyncMultiplex, this);

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

DWORD WINAPI IFrameClientMuxer::AsyncMultiplex(void* Param)
{
	IFrameClientMuxer* This = (IFrameClientMuxer*) Param;
	This->Multiplex();
	
	return 0;
}

void IFrameClientMuxer::Multiplex()
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

	byte* buffer = NULL;
	int bufferId = -1;
	int bufferSize = -1;
	while(true)
	{
		
		if (_currFrameNum % _gop == 0)
		{
			// Get the next i frame
			EnterCriticalSection(&_iFrameCS);
			{
				while(!_iFrameBuffers->GetStart((void**)&buffer, &bufferSize, &bufferId))
					SleepConditionVariableCS(&_iFrameWaitForFrameCV, &_iFrameCS, INFINITE);
			}
			LeaveCriticalSection(&_iFrameCS);

			// Send out the i frame
			SendFrameToLocalDecoder((char*)buffer, bufferSize);

			// Finish consuming the iframe
			EnterCriticalSection(&_iFrameCS);
			{
				_iFrameBuffers->GetEnd(bufferId);
			}
			WakeConditionVariable(&_iFrameWaitForSpaceCV);
			LeaveCriticalSection(&_iFrameCS);

		} else 
		{

			// Retrieve a pframe from the circular buffer
			EnterCriticalSection(&_pFrameCS);
			{
				while(!_pFrameBuffers->GetStart((void**)&buffer, &bufferSize, &bufferId))
					SleepConditionVariableCS(&_pFrameWaitForFrameCV, &_pFrameCS, INFINITE);
			}
			LeaveCriticalSection(&_pFrameCS);

			// Now that we have a pFrame buffer, send it out to the local decoder
			SendFrameToLocalDecoder((char*)buffer, bufferSize);

			// We have finished sending it out, so let's notify the circular buffer
			EnterCriticalSection(&_pFrameCS);
			{
				_pFrameBuffers->GetEnd(bufferId);
			}
			WakeConditionVariable(&_pFrameWaitForSpaceCV);
			LeaveCriticalSection(&_pFrameCS);
		}


		_currFrameNum++;
	}

}

bool IFrameClientMuxer::SendFrameToLocalDecoder(char* frame, int size)
{
	int sent = 0;
	while (sent < size)
	{
		sent += send(_socketToDecoder, frame+sent, size, 0);
	}

	return true;
}

void IFrameClientMuxer::ReceiveIFrame(void* frame, int size)
{
	void* buffer; 
	int bufferId = -1;
	EnterCriticalSection(&_iFrameCS);
	{
		while(!_iFrameBuffers->PutStart(&buffer, &bufferId))
			SleepConditionVariableCS(&_iFrameWaitForSpaceCV, &_iFrameCS, INFINITE);
	}
	LeaveCriticalSection(&_iFrameCS);

	memcpy(buffer, frame, size);

	EnterCriticalSection(&_iFrameCS);
	{
		_iFrameBuffers->PutEnd(bufferId, size);
	}
	WakeConditionVariable(&_iFrameWaitForFrameCV);
	LeaveCriticalSection(&_iFrameCS);
}

void IFrameClientMuxer::ReceivePFrame()
{
	byte* buffer = NULL;
	int bufferId = -1;
	
	// Wait until we get a valid buffer to store stuff in
	EnterCriticalSection(&_pFrameCS);
	{
		while(!_pFrameBuffers->PutStart((void**)&buffer, &bufferId))
			SleepConditionVariableCS(&_pFrameWaitForSpaceCV, &_pFrameCS, INFINITE);
	}
	LeaveCriticalSection(&_pFrameCS);

	// Retrieve the size of the P frame
	int length = 0;
	if (recv(_socketToServer, (char*)&length, sizeof(int), 0) == SOCKET_ERROR)
	{
		char errorMsg[100];
		int errorCode = WSAGetLastError();
		sprintf_s(errorMsg,"Unable to receive P frame size. Error code: %d",errorCode);
		KahawaiLog(errorMsg, KahawaiError);
		return;
	}

	// Now retrieve the P frame completely
	int receivedBytes = 0;
	while(receivedBytes < length)
	{
		int burst = 0;
		burst = recv(_socketToServer, (char*)buffer+receivedBytes, length, 0);
		VERIFY(burst != SOCKET_ERROR);
		receivedBytes += burst;
	}

	// Now let the circular buffer know we have the buffer filled out
	EnterCriticalSection(&_pFrameCS);
	{
		_pFrameBuffers->PutEnd(bufferId, length);
	}
	WakeConditionVariable(&_pFrameWaitForFrameCV);
	LeaveCriticalSection(&_pFrameCS);
}

#endif