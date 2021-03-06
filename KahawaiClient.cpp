#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "KahawaiClient.h"

//Supported decoders
#include "FFMpegDecoder.h"
#include "MediaFoundationDecoder.h"

bool KahawaiClient::isClient() {
	return true;
}

bool KahawaiClient::isMaster() {
	return false;
}

bool KahawaiClient::isSlave() {
	return false;
}

bool KahawaiClient::Initialize()
{
	if(!Kahawai::Initialize())
		return false;

	//Obtain server IP Address (Port is common to server and client. Obtained in base class)
	_configReader->ReadProperty(CONFIG_SERVER,CONFIG_SERVER_ADDRESS, _serverIP);

	//Initialize the decoder
	if (!InitializeDecoder())
	{
		KahawaiLog("Decoder not initialized", KahawaiError);
		return false;
	}

	//Config the decoder to save (or not) the decoded frames
	_decoder->EnableFrameLogging(_saveCaptures);

	return true;
}

bool KahawaiClient::InitializeDecoder()
{
	char varBuffer[30];

	//Initialize Decoder
	_configReader->ReadProperty(CONFIG_OFFLOAD,CONFIG_DECODER, varBuffer);

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

	return (_decoder != NULL);
}

bool KahawaiClient::Finalize()
{

	KahawaiLog("Shutting down Kahawai", KahawaiDebug);

	bool cleanExit = true; 
	_offloading = false;
	cleanExit &= Kahawai::Finalize();
#ifndef NO_HANDLE_INPUT
	cleanExit &= _inputHandler->Finalize();
#endif
	_finished = true;

	return cleanExit;
}

/**
 * Executes the client side of the pipeline
 * Transform->Decode -> Show
 */
void KahawaiClient::OffloadAsync()
{
	//NOTE: The subclasses _MUST_ initialize the input handler and connect it
#ifndef NO_HANDLE_INPUT
	assert(_inputHandler != NULL && _inputHandler->IsConnected());
#endif

	//////////////////////////////////////////////////////////////////////////
	//Kahawai Client LifeCyle
	//////////////////////////////////////////////////////////////////////////
	while(_offloading)
	{

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::KAHAWAI_BEGIN, _kahawaiFrameNum);
#endif

		//Exits on error
		_offloading &= Transform(_width,_height, _kahawaiFrameNum);
		_offloading &= Decode();
		_offloading &= Show();

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase(Phase::KAHAWAI_END, _kahawaiFrameNum);
#endif
		_kahawaiFrameNum++;
	}
	/////////////////////////////////////////////////////////////////////////

	//clean up all threads and missing resources
	Finalize();

	KahawaiLog("Offload finished.\n", KahawaiDebug);
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

void KahawaiClient::GameStart()
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::GAME_BEGIN, _gameFrameNum);
#endif
}

void KahawaiClient::GameEnd()
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::GAME_END, _gameFrameNum);
#endif
	_gameFrameNum++;
}


KahawaiClient::KahawaiClient(void)
	:Kahawai(),
	_decoder(0),
	_inputHandler(NULL),
	_gameFrameNum(0)
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

	if (_measurement!=NULL)
		delete _measurement;

	_measurement = NULL;
}


#endif
