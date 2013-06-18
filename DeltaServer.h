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

	//Lifecycle methods
	void	OffloadAsync();
	bool	Initialize();
	bool	Finalize();

	bool	Capture(int width, int height, void* args);
	bool	Transform(int width, int height);
	int		Encode(void** transformedFrame);
	bool	Send(void** compressedFrame, int frameSize);
	
	//Input Handling
	void*	HandleInput();
	int		GetFirstInputFrame();


	//Fidelity configuration
	bool	IsHD();


	//ADDED BY KIRON
	bool isClient();
	bool isMaster();
	bool isSlave();

private:
	bool	InitMapping();

	bool theMaster;
	bool theSlave;
};

byte Delta(byte hi, byte lo);
