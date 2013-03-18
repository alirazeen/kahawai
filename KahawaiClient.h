#pragma once
#include "kahawai.h"
#include "ConfigReader.h"
#include "VideoDecoder.h"
#include "InputHandlerClient.h"
#include "Measurement.h"
#include <queue>
using namespace std;

class KahawaiClient :
	public Kahawai
{
protected:

	//////////////////////////////////////////////////////////////////////////
	// Public interface (must be implemented by derived classes)
	//////////////////////////////////////////////////////////////////////////
	//Pipeline (Interface)
	virtual bool		Decode()=0;
	virtual bool		Show()=0;


	//////////////////////////////////////////////////////////////////////////
	///Public Client Methods
	//////////////////////////////////////////////////////////////////////////

	bool				Initialize();
	bool				Finalize();

	//Game fidelity
	bool				IsHD();

	//Asynchronous entry point
	void				OffloadAsync();

	//Input delay profile
	void				PingServer();
	void				ProfileGraphics();
	int					GetDisplayedFrames();
	void*				HandleInput(void*);
	bool				IsInputSource();

	//////////////////////////////////////////////////////////////////////////
	//Member variables
	//////////////////////////////////////////////////////////////////////////
	char				_serverIP[75]; //Only client specifies IP. Port specified in base class
	VideoDecoder*		_decoder;

	//Input Handling
	InputHandlerClient* _inputHandler;
	int					_maxFPS;
	int					_serverRTT;
	queue<void*>		_localInputQueue;

	//Instrumentation
	Measurement*		_measurement;
private:
	void*				_lastCommand;

public:
	KahawaiClient(void);
	~KahawaiClient(void);

	//public Interface

	//Instrumentation
	void				FrameStart();
	void				FrameEnd();

};

