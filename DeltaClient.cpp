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

	//Initialize input handler
#ifndef NO_HANDLE_INPUT
	_inputHandler = new InputHandlerClient(_serverIP,_serverPort+10,_gameName);
#endif

	//Read client's resolution. (Can be lower than the server's. Interpolation occurs to match deltas)
	_clientWidth = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_WIDTH);
	_clientHeight = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_HEIGHT);
	
	//Re-Initialize sws scaling context and capture frame buffer (client may have different resolution)
	delete[] _sourceFrame;
	_convertCtx = sws_getContext(_clientWidth,_clientHeight,PIX_FMT_BGRA, _width, _height,PIX_FMT_YUV420P, SWS_FAST_BILINEAR,NULL,NULL,NULL);
	_sourceFrame = new uint8_t[_clientWidth*_clientWidth*SOURCE_BITS_PER_PIXEL];

	_measurement = new Measurement("delta_client_measurement.csv");

	return true;
}

bool DeltaClient::ShouldSkip()
{
	// Frames are never skipped from rendering in
	// the delta technique
	return false;
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
	_measurement->CaptureStart(_renderedFrames+1);
	bool result = KahawaiClient::Capture(_clientWidth,_clientHeight);
	_measurement->CaptureEnd(_renderedFrames);
	
	return result;
}


/**
 * Transforms the captured screen from RGB to YUV420p
 * Overrides base class transform because the client may have a lower resolution
 * @param height the height otimf the screen to be captured
 * @param width the width of the screen to be captured
 * @return true if the transformation is successful
 */
bool DeltaClient::Transform(int width, int height)
{
	//transforms the screen captured at the client resolution
	_measurement->TransformStart();
	bool result = KahawaiClient::Transform(_clientWidth, _clientHeight);
	_measurement->TransformEnd();

	return result;
}

bool DeltaClient::Decode()
{	
	_measurement->DecodeStart();
	LogYUVFrame(_saveCaptures,"low",_renderedFrames,(char*)_transformPicture->img.plane[0],_clientWidth,_clientHeight);
	bool result = _decoder->Decode(Patch,_transformPicture->img.plane[0]);
	_measurement->DecodeEnd();

	return result;
}

bool DeltaClient::Show()
{
	_measurement->ShowStart();
	bool result = _decoder->Show();
	_measurement->ShowEnd();

	return result;
}


//Input Handling

/**
 * Returns the first frame that should receive user's input
 * due to offloading delay effect
 * @return the first frame that should receive input
 */
int DeltaClient::GetFirstInputFrame()
{
	//TODO: Need to give a real value based on profiling
	return 3;
}


#endif