#pragma once
#include "KahawaiClient.h"
#include "CircularBuffer.h"

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
	void		ReceiveIFrame(void* frame, int size);

private:

	//Needed for bookkeeping
	int		_currFrameNum;
	int		_gop;

	//Buffers to hold the i- and p-frames received from the 
	//local encoder and remote server
	CircularBuffer*		_iFrameBuffers;
	int					_iFrameMaxSize; // Max iFrame size
	
	CircularBuffer*		_pFrameBuffers;
	int					_pFrameMaxSize; // Max pFrame size

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
	CRITICAL_SECTION	_initSocketCS;
	CONDITION_VARIABLE	_initSocketCV;

	CRITICAL_SECTION	_iFrameCS;
	CONDITION_VARIABLE	_iFrameWaitForFrameCV;
	CONDITION_VARIABLE	_iFrameWaitForSpaceCV;

	CRITICAL_SECTION	_pFrameCS;
	CONDITION_VARIABLE	_pFrameWaitForFrameCV;
	CONDITION_VARIABLE	_pFrameWaitForSpaceCV;

	//Connect to the cloud server
	bool		InitSocketToServer();

	//Listen locally for a connection from the
	//IFrameClient
	static DWORD WINAPI		AsyncInitLocalSocket(void* Param);
	bool					InitLocalSocket();

	//Methods to send frames to the local decoder
	static DWORD WINAPI		AsyncSendFrames(void* Param);
	void					SendFrames();

	//Methods to receive P-frames from the remote server
	static DWORD WINAPI		AsyncReceivePFrames(void* Param);
	void					ReceivePFrame();

	//Send a frame to the local decoder
	bool		SendFrameToLocalDecoder(char* frame, int size);
};

