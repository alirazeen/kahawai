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
	bool		Capture(int width, int height, void* args);
	bool		Transform(int width, int height, int frameNum);
	bool		Decode();
	bool		Show();

	//Input Handling Profile
	void*		HandleInput();
	int			GetFirstInputFrame();

private:
	void*	_lastCommand;

	CRITICAL_SECTION	_inputSocketCS;
	CONDITION_VARIABLE	_inputSocketCV;
	bool				_inputConnectionDone;

	int		_numInputSampled;
	int		_numInputProcessed;
};

byte Patch(byte delta, byte lo);
