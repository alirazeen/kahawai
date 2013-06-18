#pragma once
#include "kahawaiclient.h"
class DeltaClient :
	public KahawaiClient
{
public:
	DeltaClient(void);
	~DeltaClient(void);

protected:
	//Low fidelity resolution
	int			_clientWidth;
	int			_clientHeight;

	bool		StartOffload();
	bool		Offload(uint8_t* frameBuffer);
	bool		Initialize();

	//Kahawai Lifecycle
	void		OffloadAsync();
	bool		Capture(int width, int height);
	bool		Transform(int width, int height);
	bool		Decode();
	bool		Show();

	//Input Handling Profile
	void*		HandleInput();
	int			GetFirstInputFrame();

private:
	void*	_lastCommand;

	CRITICAL_SECTION	_socketCS;
	CONDITION_VARIABLE	_socketCV;
	bool				_connectionAttemptDone;

};

byte Patch(byte delta, byte lo);
