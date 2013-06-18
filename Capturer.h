#pragma once
#include <stdint.h>
#include "utils.h"

class Capturer
{
public:
	virtual ~Capturer(void) {}
	virtual uint8_t* CaptureScreen(void* args) = 0;
};

