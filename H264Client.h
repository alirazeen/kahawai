#pragma once
#include "kahawaiclient.h"
class H264Client :
	public KahawaiClient
{
public:
	H264Client(void);
	~H264Client(void);

protected:
	int			_clientWidth;
	int			_clientHeight;

	bool		StartOffload();
	void		OffloadAsync();
	bool		Initialize();

	bool		Capture(int width, int height);
	bool		Transform(int width, int height);
	bool		Decode();
	bool		Show();	

	bool		StopOffload();

	int			GetFirstInputFrame();
	void*		HandleInput();

private:
	void*	_lastCommand;

	CRITICAL_SECTION	_socketCS;
	CONDITION_VARIABLE	_socketCV;
	bool				_connectionAttemptDone;

};

