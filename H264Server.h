#pragma once
#include "kahawaiserver.h"
class H264Server :
	public KahawaiServer
{
public:
	H264Server(void);
	~H264Server(void);

protected:
	//Kahawai Lifecycle
	bool Initialize();
	int Encode(void** compressedFrame);
	bool Send(void** compressedFrame, int frameSize);
};

