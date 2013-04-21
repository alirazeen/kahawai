#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "KahawaiServer.h"

/**
 * Executes the server side of the pipeline
 * Transform->Encode->Send
 */
void KahawaiServer::OffloadAsync()
{
	void* compressedFrame = NULL;

	//////////////////////////////////////////////////////////////////////////
	//Kahawai Server LifeCyle
	//////////////////////////////////////////////////////////////////////////
	while(_offloading)
	{
#ifndef MEASUREMENT_OFF
		_measurement->KahawaiStart();
#endif
		
		_offloading	 &=	Transform(_width,_height);
		int frameSize = Encode(&compressedFrame);
		_offloading	 &=	Send(&compressedFrame,frameSize);
		
#ifndef MEASUREMENT_OFF
		_measurement->KahawaiEnd();
#endif
	}
	/////////////////////////////////////////////////////////////////////////

	Finalize();

	KahawaiLog("Finished Offloading", KahawaiDebug);
}

bool KahawaiServer::Initialize()
{
	if(!Kahawai::Initialize())
		return false;

	char varBuffer[30];

	//Initialize Encoder
	_configReader->ReadProperty(CONFIG_OFFLOAD,CONFIG_ENCODER, varBuffer);
	
	//Use X264 Encoder
	if(_strnicmp(varBuffer,CONFIG_X264_ENCODER,sizeof(CONFIG_X264_ENCODER))==0)
	{
		_preset = _configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_ENCODER_LEVEL);
		_crf = _configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_CRF);
		//Actual initialization depends on whether is IFrame or Delta
	}
	else
	{
		KahawaiLog("Invalid Configuration:Unsupported Encoder", KahawaiError);
		return false;
	}

	return true;
}

bool KahawaiServer::Finalize()
{

	KahawaiLog("Shutting down Kahawai", KahawaiDebug);

	bool cleanExit = true; 
	_offloading = false;
	cleanExit &= Kahawai::Finalize();
	cleanExit &= _inputHandler->Finalize();
	_finished = true;

	return cleanExit;
}

bool KahawaiServer::IsHD()
{
	return true;
}

void KahawaiServer::FrameStart()
{
#ifndef MEASUREMENT_OFF
	_measurement->FrameStart();
#endif
}

void KahawaiServer::FrameEnd()
{
#ifndef MEASUREMENT_OFF
	_measurement->FrameEnd();
#endif
}

void* KahawaiServer::HandleInput(void*)
{
	throw 1;
}

//The server is never the source of the input
bool KahawaiServer::IsInputSource()
{
	return false;
}

int KahawaiServer::GetDisplayedFrames()
{
	return _renderedFrames;
}


KahawaiServer::KahawaiServer(void)
	:Kahawai(),
	_encoder(NULL),
	_inputHandler(NULL),
	_crf(0),
	_preset(0)

{
}


KahawaiServer::~KahawaiServer(void)
{
	if(_encoder!=NULL)
		delete _encoder;
	_encoder = NULL;
}

#endif
