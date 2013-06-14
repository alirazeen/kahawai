#pragma once
#include "kahawaiclient.h"
class H264Client :
	public KahawaiClient
{
public:
	H264Client(void);
	~H264Client(void);

protected:
	bool		Capture(int width, int height);
	bool		Transform(int width, int height);
	bool		Decode();
	bool		Show();	

	bool		StopOffload();

	int			GetFirstInputFrame();
	void		WaitForInputHandling();
	void*		HandleInput();


};

