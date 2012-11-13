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

	return true;
}


/**
 * Executes the client side of the pipeline
 * Transform->Decode -> Show
 */
void KahawaiClient::OffloadAsync()
{
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

bool KahawaiClient::IsHD()
{
	return false;
}


KahawaiClient::KahawaiClient(void)
	:Kahawai(),
	_decoder(0)
{
	strncpy(_serverIP,KAHAWAI_LOCALHOST,sizeof(KAHAWAI_LOCALHOST));

}


KahawaiClient::~KahawaiClient(void)
{
	if(_decoder!=NULL)
		delete _decoder;

	_decoder=NULL;
}

#endif