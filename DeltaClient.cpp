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
	_clientWidth(0),
	_lastCommand(NULL)
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
	_inputHandler = new InputHandlerClient(_serverIP,_serverPort+PORT_OFFSET_INPUT_HANDLER,_gameName);
#endif

	//Read client's resolution. (Can be lower than the server's. Interpolation occurs to match deltas)
	_clientWidth = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_WIDTH);
	_clientHeight = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_HEIGHT);
	
	//Re-Initialize sws scaling context and capture frame buffer (client may have different resolution)
	delete[] _sourceFrame;
	_convertCtx = sws_getContext(_clientWidth,_clientHeight,PIX_FMT_BGRA, _width, _height,PIX_FMT_YUV420P, SWS_FAST_BILINEAR,NULL,NULL,NULL);
	_sourceFrame = new uint8_t[_clientWidth*_clientWidth*SOURCE_BITS_PER_PIXEL];

#ifndef MEASUREMENT_OFF
	_measurement = new Measurement("delta_client_measurement.csv");
#endif

	return true;
}

void DeltaClient::OffloadAsync()
{
	//Connect input handler to server
#ifndef NO_HANDLE_INPUT
	if(!_inputHandler==NULL && !_inputHandler->Connect())
	{
		KahawaiLog("Unable to start input handler", KahawaiError);
		_offloading = false;
		return;
	}
#endif

	KahawaiClient::OffloadAsync();
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
	
#ifndef MEASUREMENT_OFF
	_measurement->CaptureStart(_renderedFrames+1);
#endif

	//Captures at the client resolution
	bool result = KahawaiClient::Capture(_clientWidth,_clientHeight);
	
#ifndef MEASUREMENT_OFF
	_measurement->CaptureEnd(_renderedFrames);
#endif

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
	

#ifndef MEASUREMENT_OFF
	_measurement->TransformStart();
#endif
	
	bool result = KahawaiClient::Transform(_clientWidth, _clientHeight);
	
#ifndef MEASUREMENT_OFF
	_measurement->TransformEnd();
#endif

	return result;
}

bool DeltaClient::Decode()
{	
#ifndef MEASUREMENT_OFF
	_measurement->DecodeStart();
#endif

	LogYUVFrame(_saveCaptures,"low",_renderedFrames,(char*)_transformPicture->img.plane[0],_clientWidth,_clientHeight);
	bool result = _decoder->Decode(Patch,_transformPicture->img.plane[0]);

#ifndef MEASUREMENT_OFF
	_measurement->DecodeEnd();
#endif

	return result;
}

bool DeltaClient::Show()
{
#ifndef MEASUREMENT_OFF
	_measurement->ShowStart();
#endif
	
	bool result = _decoder->Show();
	
#ifndef MEASUREMENT_OFF
	_measurement->ShowEnd();
#endif

	return result;
}



//////////////////////////////////////////////////////////////////////////
//INPUT Handling
//////////////////////////////////////////////////////////////////////////


void* DeltaClient::HandleInput(void* inputCommand)
{
	//Free memory from previous invocations
	if(_lastCommand != NULL)
	{
		delete[] _lastCommand;
		_lastCommand = NULL;
	}


	//Create a copy of the command to push into the queue
	size_t cmdLength = _inputHandler->GetCommandLength();

	char* queuedCommand = new char[cmdLength];
	memcpy(queuedCommand,inputCommand,cmdLength);


	_localInputQueue.push(queuedCommand);
	_inputHandler->SendCommand(queuedCommand);

#ifndef MEASUREMENT_OFF
	_measurement->InputReceived(queuedCommand, _renderedFrames+1); // +1 because _renderedFrames is one step behind at this point
#endif

	if(!ShouldHandleInput())
	{
		return _inputHandler->GetEmptyCommand();
	}
	else
	{
		_lastCommand = _localInputQueue.front();
	
#ifndef MEASUREMENT_OFF
		_measurement->InputSent(_lastCommand, _renderedFrames+1);
#endif

		_localInputQueue.pop();
		return _lastCommand;
	}	
}

/**
 * Returns the first frame that should receive user's input
 * due to offloading delay effect
 * @return the first frame that should receive input
 */
int DeltaClient::GetFirstInputFrame()
{
	// This is actually the frame gap. 
	// TODO: This should actually be read from a config file
	// or dynamically determined based on the RTT or some 
	// combination of the two. It should NOT be a static value
	return 3;
}


#endif