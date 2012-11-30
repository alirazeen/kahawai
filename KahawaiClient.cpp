#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "KahawaiClient.h"

//Supported decoders
#include "FFMpegDecoder.h"
#include "MediaFoundationDecoder.h"

bool KahawaiClient::Initialize()
{
	if(!Kahawai::Initialize())
		return false;

	char varBuffer[30];

	//Initialize Decoder
	_configReader->ReadProperty(CONFIG_OFFLOAD,CONFIG_DECODER, varBuffer);

	//Obtain server IP Address (Port is common to server and client. Obtained in base class)
	_configReader->ReadProperty(CONFIG_SERVER,CONFIG_SERVER_ADDRESS, _serverIP);

	//Use FFMPEG Decoder
	if(_strnicmp(varBuffer,CONFIG_FFMPEG_DECODER,sizeof(CONFIG_FFMPEG_DECODER))==0)
	{
		// Open video file
		char url[100];
		sprintf_s(url,100,"tcp://%s:%d",_serverIP,_serverPort);
		_decoder = new FFMpegDecoder(url,_width, _height);
	}

	//Use Microsoft Media Foundation Decoder
	if(_strnicmp(varBuffer,CONFIG_MEDIA_FOUNDATION_DECODER,sizeof(CONFIG_MEDIA_FOUNDATION_DECODER))==0)
	{
		_decoder = new MediaFoundationDecoder();
	}

	//Config the decoder to save (or not) the decoded frames
	_decoder->EnableFrameLogging(_saveCaptures);

	//Initialize input handler
	_inputHandler = new InputHandlerClient(_serverIP,_serverPort+10,_gameName);


	return true;
}


/**
 * Executes the client side of the pipeline
 * Transform->Decode -> Show
 */
void KahawaiClient::OffloadAsync()
{
	//Connect input handler to server
#ifndef HANDLE_INPUT
	if(!_inputHandler->Connect())
	{
		KahawaiLog("Unable to start input handler", KahawaiError);
		_offloading = false;
		return;
	}
#endif
	//////////////////////////////////////////////////////////////////////////
	//Kahawai Client LifeCyle
	//////////////////////////////////////////////////////////////////////////
	while(_offloading)
	{
		//Exits on error
		_offloading &= Transform(_width,_height);
		_offloading &= Decode();
		_offloading &= Show();
	}
	/////////////////////////////////////////////////////////////////////////

	KahawaiLog("Offload finished.\n", KahawaiDebug);
}


//////////////////////////////////////////////////////////////////////////
//INPUT Handling
//////////////////////////////////////////////////////////////////////////


void* KahawaiClient::HandleInput(void* inputCommand)
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

//The client is the one receiving direct input from the user
bool KahawaiClient::IsInputSource()
{
	return true;
}

bool KahawaiClient::IsHD()
{
	return false;
}

int KahawaiClient::GetDisplayedFrames()
{
	return _renderedFrames;
	//return _decoder->GetDisplayedFrames();
}


KahawaiClient::KahawaiClient(void)
	:Kahawai(),
	_decoder(0),
	_lastCommand(NULL)
{
	strncpy_s(_serverIP,KAHAWAI_LOCALHOST,sizeof(KAHAWAI_LOCALHOST));

}

KahawaiClient::~KahawaiClient(void)
{
	if(_decoder!=NULL)
		delete _decoder;

	_decoder=NULL;

	if(_inputHandler!=NULL)
		delete _inputHandler;

	_inputHandler = NULL;
}


#endif