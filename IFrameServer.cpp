#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameServer.h"

//Supported encoders
#include "X264Encoder.h"

IFrameServer::IFrameServer(void)
	:_gop(0),
	_currFrameNum(0)
{
}


IFrameServer::~IFrameServer(void)
{
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
	_measurement = new Measurement("iframe_server.csv");
	_inputHandler->SetMeasurement(_measurement);
#endif // MEASUREMENT_OFF

	return (_encoder!=NULL && _inputHandler!=NULL);
}

void IFrameServer::OffloadAsync()
{
	bool connection = false;

	//Initialize input handler
#ifndef NO_HANDLE_INPUT
	connection = _inputHandler->Connect();
#endif

	_socketToClient = CreateSocketToClient(_serverPort);

	if(_socketToClient==INVALID_SOCKET || !connection)
	{
		KahawaiLog("Unable to create connection to client in IFrameServer::OffloadAsync()", KahawaiError);
		return;
	}

	KahawaiServer::OffloadAsync();
}

bool IFrameServer::Capture(int width, int height)
{

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::CAPTURE_BEGIN, _gameFrameNum);
#endif // MEASUREMENT_OFF

	bool result = KahawaiServer::Capture(width,height);

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::CAPTURE_END, _gameFrameNum);
#endif // MEASUREMENT_OFF

	return result;
}

bool IFrameServer::Transform(int width, int height)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_BEGIN, _kahawaiFrameNum);
#endif // MEASUREMENT_OFF

	bool result = KahawaiServer::Transform(width,height);
	
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_END, _kahawaiFrameNum);
#endif // MEASUREMENT_OFF

	return result;
}

int IFrameServer::Encode(void** compressedFrame)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::ENCODE_BEGIN, _kahawaiFrameNum);
#endif // MEASUREMENT_OFF

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
#endif // MEASUREMENT_OFF

	return result;
}

bool IFrameServer::Send(void** compressedFrame, int frameSize)
{

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::SEND_BEGIN, _kahawaiFrameNum);
#endif // MEASUREMENT_OFF

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
#endif // MEASUREMENT_OFF

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

	_inputHandler->ReceiveCommand();
}


int IFrameServer::GetFirstInputFrame()
{
	return FRAME_GAP;
}


#endif