#pragma once
#include "kahawaiBase.h"

class Measurement;
class VideoEncoder
{
public:
	virtual ~VideoEncoder(void) {}

	/**
	 * General interface for encoding video, regardless of implementation 
	 * @param pictureIn the original source
	 * @param pictureOut the encoded result
	 * @return the size in bytes of pictureOut. Zero or less on error
	 */
	virtual int Encode(void* pictureIn, void** pictureOut, kahawaiTransform apply = 0, byte* base = 0) = 0; 
	virtual int GetBlackFrame(SwsContext* convertCtx, void** pictureOut){return -1;}

	void SetMeasurement(Measurement* measurement) {_measurement = measurement;}

protected:
	int _numEncodedFrames;
	Measurement* _measurement;

	VideoEncoder() {_numEncodedFrames = 0;}
};

