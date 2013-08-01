#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "H264Server.h"

//Supported encoders
#include "X264Encoder.h"

H264Server::H264Server(void)
	:KahawaiServer(),
	_connectionAttemptDone(false)
{
}


H264Server::~H264Server(void)
{
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
	_inputHandler->SetMeasurement(_measurement);
#endif

	InitializeCriticalSection(&_socketCS);
	InitializeConditionVariable(&_socketCV);

	return true;
}

bool H264Server::StartOffload()
{
	bool result = KahawaiServer::StartOffload();

	if (result) //TODO && !_connectionAttemptDone
	{
		EnterCriticalSection(&_socketCS);
		{
			while(!_connectionAttemptDone)
				SleepConditionVariableCS(&_socketCV, &_socketCS, INFINITE);
		}
		LeaveCriticalSection(&_socketCS);

#ifndef NO_HANDLE_INPUT
		result = _inputHandler->IsConnected();
#endif
	}

	return result;
}

void H264Server::OffloadAsync()
{
	bool connection = false; //TODO deltaserver inits this to true
	EnterCriticalSection(&_socketCS);
	{
#ifndef NO_HANDLE_INPUT
		connection = _inputHandler->Connect();
#endif
		_connectionAttemptDone = true;
	}
	WakeConditionVariable(&_socketCV);
	LeaveCriticalSection(&_socketCS);

	_socketToClient = CreateSocketToClient(_serverPort);

	if(_socketToClient==INVALID_SOCKET || !connection)
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