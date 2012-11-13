#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "KahawaiServer.h"

/**
 * Executes the server side of the pipeline
 * Transform->Encode->Send
 */
void KahawaiServer::OffloadAsync()
{
	void** compressedFrame = new void*;


	//////////////////////////////////////////////////////////////////////////
	//Kahawai Server LifeCyle
	//////////////////////////////////////////////////////////////////////////
	while(_offloading)
	{

						Transform(_width,_height);
		int frameSize = Encode(compressedFrame);
						Send(compressedFrame,frameSize);
	}
	//////////////////////////////////////////////////////////////////////////
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
		KahawaiLog("Invalid ConfiguratioN:Unsupported Encoder", KahawaiError);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Networking
//////////////////////////////////////////////////////////////////////////

/**
 * Creates a connection to the offloading client 
 * @param host_port the port to listen to
 * @return true if the connection is successful
 */
SOCKET KahawaiServer::CreateSocketToClient(int host_port)
{
#ifndef _MSC_VER
	KahawaiLog("Kahawai Networking hasn't been implemented for non Win32 platforms",KahawaiError)
	return INVALID_SOCKET;
#endif

	//Initialize sockets and set any options
	int hsock;
	int * p_int ;
	hsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(hsock == -1){
		//"Error initializing socket %d\n",WSAGetLastError());
		return false;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
			//"Error setting options %d\n", WSAGetLastError());
			free(p_int);
			return false;
	}
	free(p_int);

	//Bind and listen
	struct sockaddr_in my_addr;

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;

	if( bind( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
		//"Error binding to socket, make sure nothing else is listening on this port %d\n",WSAGetLastError());
		return false;
	}
	if(listen( hsock, 10) == -1 ){
		//"Error listening %d\n",WSAGetLastError());
		return false;
	}	

	//Now lets to the server stuff

	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);

	return accept( hsock, (SOCKADDR*)&sadr, &addr_size);

}

bool KahawaiServer::IsHD()
{
	return true;
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
