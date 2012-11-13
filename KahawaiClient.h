#pragma once
#include "kahawai.h"
#include "ConfigReader.h"
#include "VideoDecoder.h"

class KahawaiClient :
	public Kahawai
{
protected:
	//Member variables
	char _serverIP[75]; //Only client specifies IP. Port specified in base class
	VideoDecoder* _decoder;

	///Methods
	//Pipeline
	bool Initialize();
	virtual bool Decode()=0;
	virtual bool Show()=0;

	//Game fidelity
	bool IsHD();

	//Asynchronous entry point
	void OffloadAsync();

	//Input delay profile
	virtual int			GetFirstInputFrame(); //returns the number of the first frame to receive input
	void				PingServer();
	void				ProfileGraphics();

public:
	KahawaiClient(void);
	~KahawaiClient(void);

	//public Interface

private:

	//Input profile information
	int	_maxFPS;
	int _serverRTT;


};

