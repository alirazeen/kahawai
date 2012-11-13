#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "H264Server.h"

//Supported encoders
#include "X264Encoder.h"

bool H264Server::Initialize()
{
	if(!Kahawai::Initialize())
		return false;

	//Initialize encoder
	_encoder = new X264Encoder(_height,_width,_fps,_crf,_preset);

	//Create socket to client (if master)
	_socketToClient = CreateSocketToClient(_serverPort);

	if(_socketToClient==INVALID_SOCKET)
	{
		KahawaiLog("Unable to create connection to client", KahawaiError);
		return false;
	}

	return true;
}

int H264Server::Encode(void** compressedFrame)
{
	return _encoder->Encode(_transformPicture,compressedFrame);
}

bool H264Server::Send(void** compressedFrame,int frameSize)
{
	//Send the encoded delta to the client
	if(send(_socketToClient,(char*) compressedFrame,frameSize,0)==SOCKET_ERROR)
	{
		KahawaiLog("Unable to send frame to client", KahawaiError);
		return false;
	}
}

H264Server::H264Server(void)
	:KahawaiServer()
{
}


H264Server::~H264Server(void)
{
}

#endif