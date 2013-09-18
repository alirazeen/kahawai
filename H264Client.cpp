#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "H264Client.h"


H264Client::H264Client(void)
	:KahawaiClient(),
	_lastCommand(NULL),
	_inputConnectionDone(false)
{
}


H264Client::~H264Client(void)
{
}

bool H264Client::Initialize()
{
	if (!KahawaiClient::Initialize())
		return false;

#ifndef NO_HANDLE_INPUT
	_inputHandler = new InputHandlerClient(_serverIP, _serverPort+PORT_OFFSET_INPUT_HANDLER, _gameName);
#endif

	_clientWidth = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_WIDTH);
	_clientHeight = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_HEIGHT);

#ifndef MEASUREMENT_OFF
	_measurement = new Measurement("h264_client.csv");
	KahawaiClient::SetMeasurement(_measurement);
	_inputHandler->SetMeasurement(_measurement);
	_decoder->SetMeasurement(_measurement);
#endif

	InitializeCriticalSection(&_inputSocketCS);
	InitializeConditionVariable(&_inputSocketCV);

	return true;
}

bool H264Client::StartOffload()
{
	bool result = KahawaiClient::StartOffload();

#ifndef NO_HANDLE_INPUT
	//Make sure the input handler is connected first
	//before we allow the game to continue
	if (result)
	{
		EnterCriticalSection(&_inputSocketCS);
		{
			while(!_inputConnectionDone)
				SleepConditionVariableCS(&_inputSocketCV, &_inputSocketCS, INFINITE);
		}
		LeaveCriticalSection(&_inputSocketCS);

		result = _inputHandler->IsConnected();
	}
#endif


	return result;
}

void H264Client::OffloadAsync()
{
#ifndef NO_HANDLE_INPUT
	//Connect input handler to server
	EnterCriticalSection(&_inputSocketCS);
	{
		_inputHandler->Connect();
		_inputConnectionDone = true;
	}
	WakeConditionVariable(&_inputSocketCV);
	LeaveCriticalSection(&_inputSocketCS);

	if(!_inputHandler->IsConnected())
	{
		KahawaiLog("Unable to start input handler", KahawaiError);
		_offloading = false;
		return;
	}
#endif

	KahawaiClient::OffloadAsync();
}

/**
 *  H264 Client does not need to capture the screen
 *  Overrides default behavior (doing nothing)
 */
bool H264Client::Capture(int width, int height, void *args)
{
	return true;
}

/**
 *  H264 Client does not need to transform anything
 *  Overrides default behavior (doing nothing)
 */
bool H264Client::Transform(int width, int height, int frameNum)
{
	return true;
}

/**
 * Decodes the incoming H264 video from the server.
 * No transformation is applied
 */
bool H264Client::Decode()
{
	return _decoder->Decode(NULL,NULL);
}

/**
 * Shows the decoded video 
 */
bool H264Client::Show()
{
	if (_kahawaiFrameNum > 0)
	{
		// If we are showing anything other than the first
		// frame, grab the inputs and send it to the server
		// Do also see H264Server::GetFirstInputFrame for more details
		void* inputCommand = _fnSampleUserInput();
		_inputHandler->SendCommand(inputCommand);
	}

	return _decoder->Show();
}


bool H264Client::StopOffload()
{
	//Offload stops naturally when the server stops sending data
	return true;
}

//////////////////////////////////////////////////////////////////////////
//Input Handling
//////////////////////////////////////////////////////////////////////////

void* H264Client::HandleInput()
{
	// We won't process any inputs in this client so always return
	// the empty command
	return _inputHandler->GetEmptyCommand();
}

int H264Client::GetFirstInputFrame()
{
	// This function is not valid for the client since it does not
	// actually process any frames
	return 0;
}
//////////////////////////////////////////////////////////////////////////

#endif