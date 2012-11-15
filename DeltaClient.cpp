#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "DeltaClient.h"
#include "ConfigReader.h"


//////////////////////////////////////////////////////////////////////////
// Basic Patch decoding transformation
//////////////////////////////////////////////////////////////////////////
byte Patch(byte delta, byte lo)
{
	int resultPixel = (2 * (delta - 127)) + lo;

	if (resultPixel > 255)
		resultPixel = 255;

	if (resultPixel < 0)
		resultPixel = 0;

	return (byte)resultPixel;
}

DeltaClient::DeltaClient(void)
	:KahawaiClient(),
	_clientHeight(0),
	_clientWidth(0)
{
}


DeltaClient::~DeltaClient(void)
{
	if(_sourceFrame!=NULL)
		delete[] _sourceFrame;
}

bool DeltaClient::Initialize()
{
	//Call superclass
	if(!KahawaiClient::Initialize())
		return false;

	//Read client's resolution. (Can be lower than the server's. Interpolation occurs to match deltas)
	_clientWidth = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_WIDTH);
	_clientHeight = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_HEIGHT);
	
	//Re-Initialize sws scaling context and capture frame buffer (client may have different resolution)
	delete[] _sourceFrame;
	_convertCtx = sws_getContext(_clientWidth,_clientHeight,PIX_FMT_RGB24, _width, _height,PIX_FMT_YUV420P, SWS_FAST_BILINEAR,NULL,NULL,NULL);
	_sourceFrame = new uint8_t[_clientWidth*_clientWidth*SOURCE_BITS_PER_PIXEL];


	return true;
}

/**
 * Captures the content of the framebuffer into system memory
 * Overrides base class transform because the client may have a lower resolution
 * @param height the height of the screen to be captured
 * @param width the width of the screen to be captured
 * @return true if the transformation is successful
 */
bool DeltaClient::Capture(int width, int height)
{
	//Captures at the client resolution
	return KahawaiClient::Capture(_clientWidth,_clientHeight);
}


/**
 * Transforms the captured screen from RGB to YUV420p
 * Overrides base class transform because the client may have a lower resolution
 * @param height the height of the screen to be captured
 * @param width the width of the screen to be captured
 * @return true if the transformation is successful
 */
bool DeltaClient::Transform(int width, int height)
{
	//transforms the screen captured at the client resolution
	return KahawaiClient::Transform(_clientWidth, _clientHeight);
}

bool DeltaClient::Decode()
{	
	return _decoder->Decode(Patch,_transformPicture->img.plane[0]);
}

bool DeltaClient::Show()
{
	return _decoder->Show();
}

int DeltaClient::GetFirstInputFrame()
{
	//Need to implement this method
	throw 1;
	return 0;
}

#endif