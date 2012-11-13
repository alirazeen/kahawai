#pragma once
#include "kahawaiserver.h"
class DeltaServer :
	public KahawaiServer
{
protected:
	//Low fidelity resolution
	int _clientWidth;
	int _clientHeight;

	//Master-slave server instance sync variables
	bool _master;
	HANDLE _mutex;
	HANDLE _map;
	HANDLE _slaveBarrier;
	HANDLE _masterBarrier;
	byte* _mappedBuffer;


public:
	DeltaServer(void);
	~DeltaServer(void);

	//Lifecycle methods
	void OffloadAsync();
	bool Initialize();
	bool Transform(int width, int height);
	int Encode(void** compressedFrame);
	bool Send(void** compressedFrame, int frameSize);

	//Fidelity configuration
	bool IsHD();


private:
	bool InitMapping();
	void SyncCopies();

};

byte Delta(byte hi, byte lo);
