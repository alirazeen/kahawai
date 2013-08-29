#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "H264Server.h"

//Supported encoders
#include "X264Encoder.h"

H264Server::H264Server(void)
	:KahawaiServer(),
	_inputConnectionDone(false)
{
}


H264Server::~H264Server(void)
{
}

bool H264Server::isClient()
{
	return false;
}
bool H264Server::isSlave()
{
	return false;
}
bool H264Server::isMaster() 
{
	return false;
}

bool H264Server::Initialize()
{
	if(!KahawaiServer::Initialize())
		return false;

	//Initialize encoder
	_encoder = new X264Encoder(_height,_width,_fps,_crf,_preset);

	//Initialize input handler
#ifndef NO_HANDLE_INPUT
	_inputHandler = new InputHandlerServer(_serverPort+PORT_OFFSET_INPUT_HANDLER, _gameName);
#endif

#ifndef MEASUREMENT_OFF
	_measurement = new Measurement("h264_server.csv");
	KahawaiServer::SetMeasurement(_measurement);
	_inputHandler->SetMeasurement(_measurement);
#endif

	InitializeCriticalSection(&_inputSocketCS);
	InitializeConditionVariable(&_inputSocketCV);

	return true;
}

bool H264Server::StartOffload()
{
	bool result = KahawaiServer::StartOffload();

#ifndef NO_HANDLE_INPUT
	if (result)
	{
		EnterCriticalSection(&_inputSocketCS);
		{
			while(!_inputConnectionDone)
				SleepConditionVariableCS(&_inputSocketCV, &_inputSocketCS, INFINITE);
		}
		LeaveCriticalSection(&_inputSocketCS);

		result = _inputHandler->IsConnected();
	}
#endif

	return result;
}

void H264Server::OffloadAsync()
{
#ifndef NO_HANDLE_INPUT	
	EnterCriticalSection(&_inputSocketCS);
	{
		_inputHandler->Connect();
		_inputConnectionDone = true;
	}
	WakeConditionVariable(&_inputSocketCV);
	LeaveCriticalSection(&_inputSocketCS);

	if(!_inputHandler->IsConnected())
	{
		KahawaiLog("Unable to start input handler", KahawaiError);
		_offloading = false;
		return;
	}
#endif

	_socketToClient = CreateSocketToClient(_serverPort);
	if(_socketToClient==INVALID_SOCKET)
	{
		KahawaiLog("Unable to create connection to client", KahawaiError);
		return;
	}

	KahawaiServer::OffloadAsync();
}

int H264Server::Encode(void** compressedFrame)
{
	return _encoder->Encode(_transformPicture,compressedFrame);
}

bool H264Server::Send(void** compressedFrame,int frameSize)
{
	if(send(_socketToClient, (char*) *compressedFrame,frameSize,0)==SOCKET_ERROR)
	{
		char errorMsg[100];
		int errorCode = WSAGetLastError();
		sprintf_s(errorMsg,"Unable to send frame to client. Error code: %d",errorCode);
		KahawaiLog(errorMsg, KahawaiError);
		return false;
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////
//Input Handling
//////////////////////////////////////////////////////////////////////////

void* H264Server::HandleInput()
{
	_inputHandler->SetFrameNum(_gameFrameNum);

	if (!ShouldHandleInput())
		return _inputHandler->GetEmptyCommand();

	char* cmd = (char*) _inputHandler->ReceiveCommand();
	return cmd;
}

int H264Server::GetFirstInputFrame()
{
	return FRAME_GAP;
}

//////////////////////////////////////////////////////////////////////////

#endif
