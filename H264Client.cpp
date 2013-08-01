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
	_inputHandler->SetMeasurement(_measurement);
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
	bool result = KahawaiClient::Capture(width, height, args);
	return true;
}

/**
 *  H264 Client does not need to capture the screen
 *  Overrides default behavior (doing nothing)
 */
bool H264Client::Transform(int width, int height)
{
	bool result = KahawaiClient::Transform(_clientWidth, _clientHeight);
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
	_inputHandler->SetFrameNum(_gameFrameNum);

	// Get the actual command
	void* inputCommand = _fnSampleUserInput();

	// Free memory from previous invocations
	if(_lastCommand != NULL)
	{
		delete[] _lastCommand;
		_lastCommand = NULL;
	}

	_localInputQueue.push(inputCommand);
	_inputHandler->SendCommand(inputCommand);

	if (!ShouldHandleInput())
	{
		return _inputHandler->GetEmptyCommand();
	}
	else
	{
		_lastCommand = _localInputQueue.front();
		_localInputQueue.pop();
		return _lastCommand;
	}
}

int H264Client::GetFirstInputFrame()
{
	return FRAME_GAP;
}
//////////////////////////////////////////////////////////////////////////

#endif