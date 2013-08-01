#pragma once
#include "kahawaiserver.h"
class DeltaServer :
	public KahawaiServer
{
protected:
	//Low fidelity resolution
	int		_clientWidth;
	int		_clientHeight;

	//Master-slave server instance sync variables
	bool	_master;
	HANDLE	_mutex;
	HANDLE	_map;
	HANDLE	_slaveBarrier;
	HANDLE	_masterBarrier;
	HANDLE	_slaveInputEvent;
	HANDLE	_masterInputEvent;


	byte*	_mappedBuffer;
	byte*	_sharedInputBuffer;


public:
	DeltaServer(void);
	~DeltaServer(void);

	bool	StartOffload();

	//Lifecycle methods
	void	OffloadAsync();
	bool	Initialize();
	bool	Finalize();

	bool	Capture(int width, int height);
	bool	Transform(int width, int height);
	int		Encode(void** transformedFrame);
	bool	Send(void** compressedFrame, int frameSize);
	
	//Input Handling
	void*	HandleInput();
	int		GetFirstInputFrame();


	//Fidelity configuration
	bool	IsHD();


private:
	bool	InitMapping();

	bool				_masterInputReady;
	bool				_inputConnectionDone;
	CRITICAL_SECTION	_inputSocketCS;
	CONDITION_VARIABLE	_inputSocketCV;

};

byte Delta(byte hi, byte lo);
