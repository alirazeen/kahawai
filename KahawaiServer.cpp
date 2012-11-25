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

		_offloading	 &=	Transform(_width,_height);
		int frameSize = Encode(&compressedFrame);
		_offloading	 &=	Send(&compressedFrame,frameSize);
	}
	/////////////////////////////////////////////////////////////////////////

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
		int preset, crf;
		preset = _configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_ENCODER_LEVEL);
		crf = _configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_CRF);
		//Actual initialization depends on whether is IFrame or Delta
	}
	else
	{
		KahawaiLog("Invalid Configuration:Unsupported Encoder", KahawaiError);
		return false;
	}

	return true;
}

bool KahawaiServer::IsHD()
{
	return true;
}

void* KahawaiServer::HandleInput(void*)
{
	throw 1;
}

int KahawaiServer::GetDisplayedFrames()
{
	return _renderedFrames;
}


KahawaiServer::KahawaiServer(void)
	:Kahawai(),
	_encoder(NULL),
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
