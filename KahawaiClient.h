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
	virtual bool		InitializeDecoder();
	bool				Finalize();

	//Game fidelity
	bool				IsHD();

	//Asynchronous entry point
	void				OffloadAsync();

	//Input delay profile
	void				PingServer();
	void				ProfileGraphics();
	int					GetDisplayedFrames();
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

	//We keep track of separate counters for keeping track of the 
	//frames rendered so far instead of using existing ones (like 
	//_renderedFrames in the kahawai base class) because it makes
	//the logic simpler. We can increment these counters in the correct
	//places and not worry about whether it will affect the logic anywhere 
	//else. Given the multi-threaded nature of Kahawai, we have to resort to
	//multiple frame counters. It's not the most elegant solution but it'll have 
	//to suffice for now.
	int					_gameFrameNum; //Number of frames rendered by the game
	int					_kahawaiFrameNum; //Number of frames rendered by Kahawai

public:
	KahawaiClient(void);
	~KahawaiClient(void);

	//public Interface

	//Instrumentation
	void				GameStart();
	void				GameEnd();

};

