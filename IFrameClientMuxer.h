#pragma once
#include "KahawaiClient.h"
class IFrameClientMuxer
{
public:
	IFrameClientMuxer(void);
	~IFrameClientMuxer(void);

	bool		Initialize(ConfigReader* configReader);

	bool		ReceiveIFrame(void** compressedFrame, int size);

protected:
	//To be called just before the client begins the offloading process
	bool		BeginOffload();

	//Methods to receive P frames from the cloud server
	static DWORD WINAPI		AsyncReceivePFrames(void* Param);
	void					ReceivePFrames();

private:

	int		_currFrameNum;
	int		_gop;

	//Buffers to hold the i- and p-frames received from the 
	//local encoder and remote server
	byte*	_iFrame;
	byte*	_pFrame;


	//Variables related to the connection to the server
	//The Muxer will retrieve the P-frames sent by the server
	int			_serverPort;
	char		_serverIP[75];
	SOCKET		_socketToServer;
	
	//Listen to a connection locally. This is where the 
	//decoder would connect to and receive I/P frames
	int			_localMuxerPort;
	SOCKET		_socketToDecoder;


	CONDITION_VARIABLE	_iframeWaitingCV;
	CONDITION_VARIABLE	_pframeWaitingCV;

	CRITICAL_SECTION	_sendingFrameCS;

	//Connect to the cloud server
	bool		InitSocketToServer();

	//Listen locally for a connection from the
	//IFrameClient
	bool		InitLocalSocket();

	bool		SendFrameToLocalSocket(char* frame, int size);
};

