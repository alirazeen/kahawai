#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "H264Client.h"


H264Client::H264Client(void)
	:KahawaiClient()
{
}


H264Client::~H264Client(void)
{
}

/**
 *  H264 Client does not need to capture the screen
 *  Overrides default behavior (doing nothing)
 */
bool H264Client::Capture(int width, int height)
{

	return true;
}

/**
 *  H264 Client does not need to capture the screen
 *  Overrides default behavior (doing nothing)
 */
bool H264Client::Transform(int width, int height)
{
	return true;
}

/**
 * Decodes the incoming H264 video from the server.
 * No transformation is applied
 */
bool H264Client::Decode()
{
	return _decoder->Decode(NULL,_transformPicture->img.plane[0]);

}

/**
 * Shows the decoded video 
 */
bool H264Client::Show()
{
	return _decoder->Show();
}

int H264Client::GetFirstInputFrame()
{
	//Need to implement this method
	throw 1;
	return 0;
}

bool H264Client::StopOffload()
{
	//Offload stops naturally when the server stops sending data
	return true;
}
#endif