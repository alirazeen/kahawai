#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameServer.h"

//Supported encoders
#include "X264Encoder.h"

IFrameServer::IFrameServer(void)
	:_gop(0),
	_currFrameNum(0),
	_inputConnectionDone(false)
{
}


IFrameServer::~IFrameServer(void)
{
}

bool IFrameServer::isClient() {
	return false;
}

//master and slave are just place holders
bool IFrameServer::isSlave() {
	return false;
}

bool IFrameServer::isMaster() {
	return true;
}

bool IFrameServer::Initialize()
{
	if(!KahawaiServer::Initialize())
		return false;

	//Read GOP size (space between iframes) from Config
	_gop = _configReader->ReadIntegerValue(CONFIG_IFRAME,CONFIG_GOP_SIZE);

	//Initialize encoder
	_encoder = new X264Encoder(_height,_width,_fps,_crf,_preset,_gop);

	//Initialize input handler
#ifndef NO_HANDLE_INPUT
	_inputHandler = new InputHandlerServer(_serverPort+PORT_OFFSET_INPUT_HANDLER, _gameName);
#endif

#ifndef MEASUREMENT_OFF
	_measurement = new Measurement("iframe_server.csv", "gop=%d\n", _gop);
	KahawaiServer::SetMeasurement(_measurement);
	_inputHandler->SetMeasurement(_measurement);
	_encoder->SetMeasurement(_measurement);
#endif

	InitializeCriticalSection(&_inputSocketCS);
	InitializeConditionVariable(&_inputSocketCV);

	return (_encoder!=NULL && _inputHandler!=NULL);
}

bool IFrameServer::StartOffload()
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

void IFrameServer::OffloadAsync()
{
	//See comments in IFrameCLient::OffloadAsync to find out why
	//we create a socket to the client before we attempt to connect
	//the input handler
	_socketToClient = CreateSocketToClient(_serverPort);
	if(_socketToClient==INVALID_SOCKET)
	{
		KahawaiLog("Unable to create connection to client in IFrameServer::OffloadAsync()", KahawaiError);
		return;
	}

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

	KahawaiServer::OffloadAsync();
}

bool IFrameServer::Capture(int width, int height, void* args)
{

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::CAPTURE_BEGIN, _gameFrameNum);
#endif

	bool result = KahawaiServer::Capture(width,height, args);

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::CAPTURE_END, _gameFrameNum);
#endif

	return result;
}

bool IFrameServer::Transform(int width, int height, int frameNum)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_BEGIN, frameNum);
#endif

	bool result = KahawaiServer::Transform(width,height, frameNum);
	
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_END, frameNum);
#endif

	return result;
}

int IFrameServer::Encode(void** compressedFrame)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::ENCODE_BEGIN, _kahawaiFrameNum);
#endif

	if(_currFrameNum%_gop==0) //is it time to encode an I-Frame
	{
		_transformPicture->i_type = X264_TYPE_IDR; //lets try with an IDR frame first
		_transformPicture->i_qpplus1 = 1;
	}
	else
	{
		_transformPicture->i_type = X264_TYPE_P; //otherwise lets encode it as a P-Frame		
		_transformPicture->i_qpplus1 = X264_QP_AUTO;
	}

	int result = _encoder->Encode(_transformPicture,compressedFrame);

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::ENCODE_END, _kahawaiFrameNum);
#endif

	return result;
}

bool IFrameServer::Send(void** compressedFrame, int frameSize)
{

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::SEND_BEGIN, _kahawaiFrameNum);
#endif

	if(_currFrameNum%_gop!=0) //We send only the P-frames
	{
		//Send the pframe size to the client
		if(send(_socketToClient, (char*)&frameSize,sizeof(frameSize),0)==SOCKET_ERROR)
		{
			char errorMsg[100];
			int errorCode = WSAGetLastError();
			sprintf_s(errorMsg,"Unable to send frame size to client. Error code: %d",errorCode);
			KahawaiLog(errorMsg, KahawaiError);
			return false;
		}

		//Send the actual pframe to the client
		if(send(_socketToClient, (char*) *compressedFrame,frameSize,0)==SOCKET_ERROR)
		{
			char errorMsg[100];
			int errorCode = WSAGetLastError();
			sprintf_s(errorMsg,"Unable to send frame to client. Error code: %d",errorCode);
			KahawaiLog(errorMsg, KahawaiError);
			return false;
		}

	}

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::SEND_END, _kahawaiFrameNum);
#endif

	_currFrameNum++;
	return true;
}

//////////////////////////////////////////////////////////////////////////
//Input Handling
//////////////////////////////////////////////////////////////////////////

void* IFrameServer::HandleInput()
{
	_inputHandler->SetFrameNum(_gameFrameNum);

	if(!ShouldHandleInput())
		return _inputHandler->GetEmptyCommand();

	int frameNum = _inputHandler->PeekCommandFrame();
	return _inputHandler->ReceiveCommand();
}


int IFrameServer::GetFirstInputFrame()
{
	//The reasons for the +3 here is similar to the reasons in
	//H264Server::GetFirstInputFrame()
	return FRAME_GAP+3;
}


#endif