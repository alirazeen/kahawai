#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "H264Server.h"

//Supported encoders
#include "X264Encoder.h"

bool H264Server::Initialize()
{
	if(!KahawaiServer::Initialize())
		return false;

	//Initialize encoder
	_encoder = new X264Encoder(_height,_width,_fps,_crf,_preset);

	return true;
}

void H264Server::OffloadAsync()
{
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
	//Send the encoded delta to the client
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

H264Server::H264Server(void)
	:KahawaiServer()
{
}


H264Server::~H264Server(void)
{
}

#endif