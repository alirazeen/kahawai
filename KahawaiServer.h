	#pragma once
#include "kahawai.h"
#include "ConfigReader.h"
#include "VideoEncoder.h"
#include "InputHandlerServer.h"
#include "Measurement.h"

class KahawaiServer :
	public Kahawai
{
protected:
	//////////////////////////////////////////////////////////////////////////
	// Server Public Interface
	//////////////////////////////////////////////////////////////////////////

	//Kahawai Lifecycle / Pipeline
	virtual int			Encode(void** transformedFrame)=0; 
	virtual bool		Send(void** compressedFrame,int frameSize)=0;

	//////////////////////////////////////////////////////////////////////////
	// Server Concrete Methods
	//////////////////////////////////////////////////////////////////////////

	bool				Initialize();
	bool				Finalize();
	//Input Handling
	void*				HandleInput(void*);
	int					GetDisplayedFrames();
	bool				IsInputSource();

	//Asynchronous entry point
	void				OffloadAsync();

	//Game Fidelity Configuration
	bool				IsHD();


	//////////////////////////////////////////////////////////////////////////
	//Member variables
	//////////////////////////////////////////////////////////////////////////

	//Encoding
	VideoEncoder*		_encoder;
	static const int	_fps = TARGET_FPS; 
	int _crf;
	int _preset;

	//Client-server communication
	SOCKET				_socketToClient;
	InputHandlerServer* _inputHandler;

	//Instrumentation
	Measurement*		_measurement;

public:
	KahawaiServer(void);
	~KahawaiServer(void);

	//Instrumentation
	virtual void		FrameStart();
	virtual void		FrameEnd();
};

