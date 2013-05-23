#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClient.h"

// Supported decoders
#include "FFMpegDecoder.h"

#define FFMPEG_DECODER_WARMUP 60

IFrameClient::IFrameClient(void) :
	_lastCommand(NULL),
	_numTransformedFrames(0)
{
	_encoderComponent = new IFrameClientEncoder();
	_muxerComponent = new IFrameClientMuxer();

	InitializeCriticalSection(&_inputCS);
	InitializeConditionVariable(&_showDoneCV);
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

	//Initialize input handler
#ifndef NO_HANDLE_INPUT
	_inputHandler = new InputHandlerClient(_serverIP,_serverPort+PORT_OFFSET_INPUT_HANDLER,_gameName);
#endif
	
#ifndef MEASUREMENT_OFF
	//Initialize instrumentation class
	_measurement = new Measurement("iframe_client.csv");
	_muxerComponent->SetMeasurement(_measurement);
	_inputHandler->SetMeasurement(_measurement);
#endif // MEASUREMENT_OFF

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

void IFrameClient::OffloadAsync()
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

	_muxerComponent->BeginOffload();
	CreateKahawaiThread(AsyncDecodeShow, this);
	
	while(_offloading) {
		_offloading &= Transform(_width, _height);
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

bool IFrameClient::Capture(int width, int height)
{
	bool result = true;
	if (!ShouldSkip())
	{
#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::CAPTURE_BEGIN,_renderedFrames);
#endif // MEASUREMENT_OFF

		result = KahawaiClient::Capture(width,height);

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::CAPTURE_END,_renderedFrames);
#endif // MEASUREMENT_OFF
	}
	else
		_renderedFrames++;

	return result;
}

bool IFrameClient::Transform(int width, int height)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_BEGIN, _numTransformedFrames);
#endif // MEASUREMENT_OFF

	bool result = KahawaiClient::Transform(width, height);
	if (result)
	{

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::IFRAME_CLIENT_ENCODE_BEGIN, _numTransformedFrames);
#endif // MEASUREMENT_OFF
		
		result = SendTransformPictureEncoder();

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::IFRAME_CLIENT_ENCODE_END, _numTransformedFrames);
#endif // MEASUREMENT_OFF
	}

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_END, _numTransformedFrames);
#endif // MEASUREMENT_OFF

	_numTransformedFrames += _gop;
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
#endif // MEASUREMENT_OFF

	bool result = _decoder->Decode();

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::DECODE_END, _kahawaiFrameNum);
#endif // MEASUREMENT_OFF

	return result;
}

bool IFrameClient::Show()
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::SHOW_BEGIN, _kahawaiFrameNum);
#endif // MEASUREMENT_OFF

	bool result = _decoder->Show();

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::SHOW_END, _kahawaiFrameNum);
#endif // MEASUREMENT_OFF

	return result;
}

void IFrameClient::WaitForInputHandling()
{
	//In the ordinary case, the client can potentially progress through the game
	//at a quicker pace than the server, as the client runs the render phase
	//only for IFrames, and runs just the logic phase for the P frames. The server,
	//on the other hand, runs both the logic/render phase for both I and P frames.
	//
	//However, we should disallow this. The client cannot progress through the game
	//at a quicker pace. When it is running the logic for a frame N, which entails
	//collecting the input from the user, it must not be allowed to progress if the
	//user has not seen the frame N-1.
	//
	//In a given frame, an input from the user makes sense semantically only if the
	//user has seen the previous frame. Hence, we should wait until the user has
	//seen the previous frame before allowing inputs to be collected. This method will
	//perform that wait
	EnterCriticalSection(&_inputCS);
	{
		//We only do the wait after FFMPEG_DECODER_WARMUP number of frames have passed
		//If FFMPEG_DECODER_WARMUP is set to 0, then a deadlock will arise because
		//the decoder is waiting for more frames in its buffer before displaying
		//what it already has and the game cannot proceed to produce more frames
		//because it is waiting for the input to be ready.
		if (_gameFrameNum > FFMPEG_DECODER_WARMUP && _gameFrameNum > _kahawaiFrameNum-1)
			SleepConditionVariableCS(&_showDoneCV, &_inputCS, INFINITE);
	}
	LeaveCriticalSection(&_inputCS);
}

void* IFrameClient::HandleInput(void* inputCommand)
{
#ifndef MEASUREMENT_OFF
	_inputHandler->SetFrameNum(_gameFrameNum);
#endif // MEASUREMENT_OFF

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

	if(!ShouldHandleInput())
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

int IFrameClient::GetFirstInputFrame()
{
	// This is actually the frame gap. 
	// TODO: This should actually be read from a config file
	// or dynamically determined based on the RTT or some 
	// combination of the two. It should NOT be a static value
	return FRAME_GAP;
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
#endif // MEASUREMENT_OFF

		offloading &= Decode();
		offloading &= Show();

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::KAHAWAI_END, _kahawaiFrameNum);
#endif // MEASUREMENT_OFF

		EnterCriticalSection(&_inputCS);
		{
			_kahawaiFrameNum++;
		}
		WakeConditionVariable(&_showDoneCV);
		LeaveCriticalSection(&_inputCS);
	}
}

#endif
