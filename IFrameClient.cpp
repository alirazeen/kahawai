#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "IFrameClient.h"

// Supported decoders
#include "FFMpegDecoder.h"

IFrameClient::IFrameClient(void)
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
	// Initialize the component that will encode the locally
	// rendered frames into I-frames
	if (!_encoderComponent->Initialize(_configReader))
		return false;

	// Initialize the component that will receive the P frames
	// from the server and mix the stream with the I frames
	if (!_muxerComponent->Initialize(_configReader))
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

}

bool IFrameClient::InitializeDecoder()
{
	char varBuffer[30];

	//Use FFMPEG Decoder
	if(_strnicmp(varBuffer,CONFIG_FFMPEG_DECODER,sizeof(CONFIG_FFMPEG_DECODER))==0)
	{
		// TODO: Fill this up appropriately based on the URL of the muxer
		// Open video file
		//char url[100];
		//sprintf_s(url,100,"tcp://%s:%d",_serverIP,_serverPort);
		//_decoder = new FFMpegDecoder(url,_width, _height);
	}


	return (_decoder != NULL);
}


bool IFrameClient::ShouldSkip()
{
	// Skip if it's not time to render an i-frame
	return (0 != _renderedFrames % _gop);
}

bool IFrameClient::Decode()
{
	KahawaiLog("Not implemented: IframeClient::Decode", KahawaiError);
	return false;
}

bool IFrameClient::Show()
{
	KahawaiLog("Not implemented: IFrameClient::Show", KahawaiError);
	return false;
}

void* IFrameClient::HandleInput(void* inputCommand)
{
	//Free memory from previous invocations
	if(_lastCommand != NULL)
	{
		delete[] _lastCommand;
		_lastCommand = NULL;
	}


	// If we're receiving a command before it's time to render an I-frame
	// we ignore the input from the user
	if (ShouldSkip())
	{
		return _inputHandler->GetEmptyCommand();
	}

	//Create a copy of the command to push into the queue
	size_t cmdLength = _inputHandler->GetCommandLength();

	char* queuedCommand = new char[cmdLength];
	memcpy(queuedCommand,inputCommand,cmdLength);


	_localInputQueue.push(queuedCommand);
	_inputHandler->SendCommand(queuedCommand);
	//_measurement->InputReceived(queuedCommand, _renderedFrames+1); // +1 because _renderedFrames is one step behind at this point

	if(!ShouldHandleInput())
	{
		return _inputHandler->GetEmptyCommand();
	}
	else
	{
		_lastCommand = _localInputQueue.front();
		//_measurement->InputSent(_lastCommand, _renderedFrames+1);
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
	return 3;
}

#endif
