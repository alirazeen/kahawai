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

	static DWORD WINAPI		AsyncReceivePFrames(void* Param);
	void					ReceivePFrames();
private:

	// Variables related to the connection to the server
	// The Muxer will retrieve the P-frames sent by the server
	int			_serverPort;
	char		_serverIP[75];
	SOCKET		_socketToServer;

	// Connect to the cloud server
	bool		InitSocketToServer();
	
	// Listen locally for a connection from the
	// IFrameClient
	bool		InitLocalSocket();
};

