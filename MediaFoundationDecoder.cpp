#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "MediaFoundationDecoder.h"
#include "KahawaiBase.h"

MediaFoundationDecoder::MediaFoundationDecoder(void)
{
}


MediaFoundationDecoder::~MediaFoundationDecoder(void)
{
}

bool MediaFoundationDecoder::Decode(transform apply, byte* patch)
{
	KahawaiLog("Unimplemented Method: FFMpegDecoder::Decode",KahawaiError);
	return false;

}

bool MediaFoundationDecoder::Show()
{

	KahawaiLog("Unimplemented Method: FFMpegDecoder::Decode",KahawaiError);
	return false;
}
#endif