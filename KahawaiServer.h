#pragma once
#include "kahawai.h"
#include "ConfigReader.h"
#include "VideoEncoder.h"

class KahawaiServer :
	public Kahawai
{
protected:
	//Kahawai Lifecycle / Pipeline
	bool Initialize();
	virtual int Encode(void** compressedFrame)=0;
	virtual bool Send(void** compressedFrame,int frameSize)=0;

	//Networking
	SOCKET CreateSocketToClient(int host_port);


	//Asynchronous entry point
	void		OffloadAsync();

	//Game Fidelity Configuration
	bool IsHD();


	//////////////////////////////////////////////////////////////////////////
	//Member variables
	//////////////////////////////////////////////////////////////////////////

	//Encoding
	VideoEncoder* _encoder;
	static const int _fps = TARGET_FPS; 
	int _crf;
	int _preset;

	//Client-server communication
	SOCKET _socketToClient;

public:
	KahawaiServer(void);
	~KahawaiServer(void);
};

