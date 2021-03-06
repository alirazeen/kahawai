#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClient.h"

// Supported decoders
#include "FFMpegDecoder.h"

IFrameClient::IFrameClient(void) :
	_lastCommand(NULL),
	_numTransformedFrames(0),
	_inputConnectionDone(false),
	_numInputProcessed(0),
	_numInputSampled(0)
{
	_encoderComponent = new IFrameClientEncoder();
	_muxerComponent = new IFrameClientMuxer();
}


IFrameClient::~IFrameClient(void)
{
	delete _encoderComponent;
	delete _muxerComponent;
}

bool IFrameClient::Initialize()
{
	// Initialize the component that will receive the P frames
	// from the server and mix the stream with the I frames
	if (!_muxerComponent->Initialize(_configReader))
		return false;

	// Initialize the component that will encode the locally
	// rendered frames into I-frames
	if (!_encoderComponent->Initialize(_configReader, _muxerComponent))
		return false;

	// Now we start initializing Kahawai. Note that we have to initialize the
	// encoder and the multiplexer before we initialize Kahawai because those 
	// components are required before we can initialize the decoder that will show
	// the final output
	if (!KahawaiClient::Initialize())
		return false;

	//Read GOP size (space between iframes) from Config
	_gop = _configReader->ReadIntegerValue(CONFIG_IFRAME,CONFIG_GOP_SIZE);

	//Set the SwsContext.
	_encoderComponent->SetConvertCtx(_convertCtx);
	_muxerComponent->SetClientEncoder(_encoderComponent);

	//Initialize input handler
#ifndef NO_HANDLE_INPUT
	_inputHandler = new InputHandlerClient(_serverIP,_serverPort+PORT_OFFSET_INPUT_HANDLER,_gameName);
#endif
	
#ifndef MEASUREMENT_OFF
	//Initialize instrumentation class
	_measurement = new Measurement("iframe_client.csv", "gop=%d\n", _gop);
	KahawaiClient::SetMeasurement(_measurement);
	_encoderComponent->SetMeasurement(_measurement);
	_muxerComponent->SetMeasurement(_measurement);
	_inputHandler->SetMeasurement(_measurement);
	_decoder->SetMeasurement(_measurement);
#endif

	InitializeCriticalSection(&_inputCS);
	InitializeConditionVariable(&_inputQueueEmptyCV);

	InitializeCriticalSection(&_inputSocketCS);
	InitializeConditionVariable(&_inputSocketCV);
	return true;
}

bool IFrameClient::InitializeDecoder()
{
	char varBuffer[30];

	//Initialize Decoder
	_configReader->ReadProperty(CONFIG_OFFLOAD,CONFIG_DECODER, varBuffer);

	//Use FFMPEG Decoder
	if(_strnicmp(varBuffer,CONFIG_FFMPEG_DECODER,sizeof(CONFIG_FFMPEG_DECODER))==0)
	{
		// Open video file
		char url[100];
		sprintf_s(url,100,"tcp://%s:%d","127.0.0.1",_serverPort+PORT_OFFSET_IFRAME_MUXER);
		_decoder = new FFMpegDecoder(url,_width, _height);
	}

	return (_decoder != NULL);
}

bool IFrameClient::IsHD()
{
	return true;
}

bool IFrameClient::StartOffload()
{
	bool result = KahawaiClient::StartOffload();

#ifndef NO_HANDLE_INPUT
	if (result)
	{
		EnterCriticalSection(&_inputSocketCS);
		{
			while (!_inputConnectionDone)
				SleepConditionVariableCS(&_inputSocketCV, &_inputSocketCS, INFINITE);
		}
		LeaveCriticalSection(&_inputSocketCS);
		result = _inputHandler->IsConnected();
	}
#endif

	return result;
}

void IFrameClient::OffloadAsync()
{
	_muxerComponent->BeginOffload();
	_decoder->WaitForConnection();

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


	CreateKahawaiThread(AsyncDecodeShow, this);
	
	while(_offloading) {
		_offloading &= Transform(_width, _height, _numTransformedFrames);
		_numTransformedFrames += _gop;
	}

	Finalize();

	KahawaiLog("Offload finished.\n", KahawaiDebug);
	//KahawaiClient::OffloadAsync();
}

bool IFrameClient::ShouldSkip()
{
	// Skip if it's not time to render an i-frame
	return (0 != _renderedFrames % _gop);
}

bool IFrameClient::Capture(int width, int height, void* args)
{
	bool result = true;
	if (!ShouldSkip())
	{
#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::CAPTURE_BEGIN,_renderedFrames);
#endif
		result = KahawaiClient::Capture(width,height, args);

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::CAPTURE_END,_renderedFrames-1);
#endif
	}
	else
		_renderedFrames++;

	return result;
}

bool IFrameClient::Transform(int width, int height, int frameNum)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_BEGIN, frameNum);
#endif

	bool result = KahawaiClient::Transform(width, height, frameNum);
	if (result)
	{

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::IFRAME_CLIENT_ENCODE_BEGIN, frameNum);
#endif
		
		result = SendTransformPictureEncoder();

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::IFRAME_CLIENT_ENCODE_END, frameNum);
#endif
	}

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_END, frameNum);
#endif

	
	return result;
}

bool IFrameClient::SendTransformPictureEncoder()
{
	return _encoderComponent->ReceiveTransformedPicture(_transformPicture);
}

bool IFrameClient::Decode()
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::DECODE_BEGIN, _kahawaiFrameNum);
#endif

	bool result = _decoder->Decode();

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::DECODE_END, _kahawaiFrameNum);
#endif

	return result;
}

bool IFrameClient::Show()
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::SHOW_BEGIN, _kahawaiFrameNum);
#endif

	bool result = _decoder->Show();

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::SHOW_END, _kahawaiFrameNum);
#endif

	return result;
}

void IFrameClient::GrabInput()
{
	EnterCriticalSection(&_inputCS);
	{
		void* inputCommand = _fnSampleUserInput();
#ifndef MEASUREMENT_OFF
		_measurement->InputSampled(_numInputSampled);
#endif
		_numInputSampled++;
		_localInputQueue.push(inputCommand);
		_inputHandler->SendCommand(inputCommand);
	}
	WakeConditionVariable(&_inputQueueEmptyCV);
	LeaveCriticalSection(&_inputCS);
}

void* IFrameClient::HandleInput()
{
	//Free memory from previous invocations
	if(_lastCommand != NULL && _lastCommand != _inputHandler->GetEmptyCommand())
	{
		delete[] _lastCommand;
		_lastCommand = NULL;
	}

	void* returnVal = NULL;

	if(!ShouldHandleInput())
	{
		return _inputHandler->GetEmptyCommand();
	}

	EnterCriticalSection(&_inputCS);
	{
		while (_localInputQueue.empty())
			SleepConditionVariableCS(&_inputQueueEmptyCV, &_inputCS, INFINITE);
		
		_lastCommand = _localInputQueue.front();
		_localInputQueue.pop();
		returnVal = _lastCommand;

#ifndef MEASUREMENT_OFF
		_measurement->InputProcessed(_numInputProcessed, _gameFrameNum);
#endif
		_numInputProcessed++;
	}
	LeaveCriticalSection(&_inputCS);

	return returnVal;
}

int IFrameClient::GetFirstInputFrame()
{
	//THe IFrameServer has a FRAME_GAP+3. Since the client processes 
	//inputs locally, both the client and the server must have the same
	//framegap so there is a +3 too.
	//
	//The reasons for the +3 in the first place is similar to the reasons
	//found in H264Server::GetFirstInputFrame()
	return FRAME_GAP+3;
}

DWORD WINAPI IFrameClient::AsyncDecodeShow(void* Param)
{
	IFrameClient* This = (IFrameClient*)Param;
	This->DecodeShow();
	return 0;
}

void IFrameClient::DecodeShow()
{
	bool offloading = true;
	while (offloading)
	{

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::KAHAWAI_BEGIN, _kahawaiFrameNum);
#endif
		
		//Ok, this is tricky. You would think that GrabInput() should appear
		//AFTER the call to Show(), but that is incorrect. Think about it. A frame X
		//is displayed on the screen until Decode/Show is done for frame X+1.
		//
		//Therefore, to grab the user's reaction to frame X, we should grab it when
		//frame X+1 is shown. The user has had time to view frame X and her
		//corresponding reaction is grabbed correctly if we do GrabInput() just before
		//decoding/showing frame X+1.
		//
		//On the OTHER hand, if we do GrabInput() AFTER the call to Show(), we're
		//grabbing inputs BEFORE the user has had time to properly observe a frame X.
		//
		//This is why in terms of line ordering, GrabInput() appears before Decode/Show.
		//Note that the _kahawaiFrameNum > FRAME_GAP, ensures that we correctly
		//collect an input for frame X when frame X+1 is about to be decoded/shown.
		if (_kahawaiFrameNum > 0)
			GrabInput();

		offloading &= Decode();
		offloading &= Show();
		

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::KAHAWAI_END, _kahawaiFrameNum);
#endif

	
		_kahawaiFrameNum++;

	}
}

#endif
