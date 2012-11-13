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
	int _clientWidth;
	int _clientHeight;


	bool Offload(uint8_t* frameBuffer);
	bool Initialize();

	//Kahawai Lifecycle
	bool Capture(int width, int height);
	bool Transform(int width, int height);
	bool Decode();
	bool Show();
};

byte Patch(byte delta, byte lo);
