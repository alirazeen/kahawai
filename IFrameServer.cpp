#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameServer.h"

//Supported encoders
#include "X264Encoder.h"

IFrameServer::IFrameServer(void)
	:_gop(0)
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

	return (_encoder!=NULL && _inputHandler!=NULL);
}

int IFrameServer::Encode(void** compressedFrame)
{
	if((_renderedFrames-1)%_gop==0) //is it time to encode an I-Frame
	{
		_transformPicture->i_type = X264_TYPE_IDR; //lets try with an IDR frame first
		_transformPicture->i_qpplus1 = 1;
	}
	else
	{
		_transformPicture->i_type = X264_TYPE_P; //otherwise lets encode it as a P-Frame		
		_transformPicture->i_qpplus1 = X264_QP_AUTO;
	}

	return _encoder->Encode(_transformPicture,compressedFrame);
}

bool IFrameServer::Send(void** compressedFrame, int frameSize)
{
	if(_renderedFrames%_gop!=0) //We send only the P-frames
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

	return true;
}

//////////////////////////////////////////////////////////////////////////
//Input Handling
//////////////////////////////////////////////////////////////////////////
void* IFrameServer::HandleInput(void* input)
{
	if(!ShouldHandleInput())
		return _inputHandler->GetEmptyCommand();

	_inputHandler->ReceiveCommand();
}


int IFrameServer::GetFirstInputFrame()
{
	return 3;
}


#endif