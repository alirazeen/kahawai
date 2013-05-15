#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClient.h"

// Supported decoders
#include "FFMpegDecoder.h"

IFrameClient::IFrameClient(void) :
	_lastCommand(NULL),
	_numTransformedFrames(0)
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

	//Initialize input handler
#ifndef NO_HANDLE_INPUT
	_inputHandler = new InputHandlerClient(_serverIP,_serverPort+PORT_OFFSET_INPUT_HANDLER,_gameName);
#endif
	
#ifndef MEASUREMENT_OFF
	//Initialize instrumentation class
	_measurement = new Measurement("iframe_client.csv");
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

bool IFrameClient::Transform(int width, int height)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_BEGIN, _numTransformedFrames);
#endif // MEASUREMENT_OFF

	bool result =  KahawaiClient::Transform(width, height);
	if (result && (_numTransformedFrames % _gop == 0))
		result = SendTransformPictureEncoder();
	

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_END, _numTransformedFrames);
#endif // MEASUREMENT_OFF

	_numTransformedFrames++;
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

void* IFrameClient::HandleInput(void* inputCommand)
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
		_measurement->AddPhase(Phase::KAHAWAI_BEGIN, _kahawaiFrameNum);
#endif // MEASUREMENT_OFF
		_kahawaiFrameNum++;
	}
}

#endif
