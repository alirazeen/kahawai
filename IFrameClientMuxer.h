#pragma once
#include "KahawaiClient.h"
class IFrameClientMuxer
{
public:
	IFrameClientMuxer(void);
	~IFrameClientMuxer(void);

	//Initialize the muxer
	bool		Initialize(ConfigReader* configReader);

	//To be called just before the client begins the offloading process
	bool		BeginOffload();

	//Receive I-frame from the local encoder
	bool		ReceiveIFrame(void** compressedFrame, int size);

private:

	//Needed for bookkeeping
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

	//Synchronization related variables
	CONDITION_VARIABLE	_iframeWaitingCV;
	CONDITION_VARIABLE	_pframeWaitingCV;

	CRITICAL_SECTION	_sendingFrameCS;

	//Connect to the cloud server
	bool		InitSocketToServer();

	//Listen locally for a connection from the
	//IFrameClient
	bool		InitLocalSocket();

	//Async method to send frames to the local decoder
	static DWORD WINAPI		AsyncSendFrames(void* Param);

	//Receive P-frame from the remote server
	void					ReceivePFrame();

	//Send a frame to the local decoder
	bool		SendFrameToLocalDecoder(char* frame, int size);
};

