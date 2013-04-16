#pragma once
#include "KahawaiClient.h"
class IFrameClientMuxer
{
public:
	IFrameClientMuxer(void);
	~IFrameClientMuxer(void);

	bool		Initialize(ConfigReader* configReader);

	bool		Decode();
	bool		Show();

protected:

	//Methods to receive P frames from the cloud server
	static DWORD WINAPI		AsyncReceivePFrames(void* Param);
	void					ReceivePFrames();

	//Methods to send the I/P frame stream to the local decoder
	static DWORD WINAPI		AsyncSendMuxedFrames(void* Param);
	void					SendMuxedFrames();

private:

	//Variables related to the connection to the server
	//The Muxer will retrieve the P-frames sent by the server
	int			_serverPort;
	char		_serverIP[75];
	SOCKET		_socketToServer;
	
	//Listen to a connection locally. This is where the 
	//decoder would connect to and receive I/P frames
	int			_localMuxerPort;
	SOCKET		_socketToDecoder;

	//Connect to the cloud server
	bool		InitSocketToServer();

	//Listen locally for a connection from the
	//IFrameClient
	bool		InitLocalSocket();
};

