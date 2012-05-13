#include<iostream>

using namespace std;

#ifdef KAHAWAI
#include "Kahawai.h"

#include<fstream>
#include<string>



/************************************************************************/
/* Kahawai Class Methods                                                */
/************************************************************************/
Kahawai::Kahawai(void)
{_x264_initialized = false;
	_encoder = NULL;
	_ffmpeg_initialized = false;
	_socketToClientInitialized = false;
	_timeInitialized = false;
	_mappedBuffer = NULL;
	_crossStreamInitialized = false;
	_renderedFrames = 0;
	_fps=60;
	_offloading = false;
	_decodedServerFrames = 0;
	_sdlInitialized = false;
	_loadedDeltaStream = false;
	_lastIFrameMerged = 0;
	_decodeThreadInitialized = false;
	_iFrame=NULL;
	_iFrameTmp=NULL;
	_iFrameSize = -1;
	_iFrameTmpSize = -1;
	_transformBuffer = NULL;
	_transformBufferTemp = NULL;
	_inputPicture = new x264_picture_t;
	_frameInPipeLine = 0;
}


Kahawai::~Kahawai(void)
{
}


bool Kahawai::InitializeSynchronization()
{
	InitializeCriticalSection(&_iFrameLock);
	InitializeCriticalSection(&_captureLock);
	InitializeConditionVariable(&_encodingCV);
	InitializeConditionVariable(&_mergingCV);
	InitializeConditionVariable(&_capturingCV);
	return true;
}

/*
=================
InitMapping
=================
*/
bool Kahawai::InitMapping(int size)
{
	HANDLE sharedFile = NULL;

	//Create File to be used for file sharing, use temporary flag for efficiency
	sharedFile = CreateFile
		(KAHAWAI_MAP_FILE,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY,
		NULL);

	if(!sharedFile)
		return false;

	//Init File Mapping Handle
	_map = CreateFileMapping
		(sharedFile,
		NULL,
		PAGE_READWRITE,
		0,
		size,
		_T("SharedFile"));

	if(!_map)
		return false;

	//Define a Mutex for accessing the file map.
	_mutex = CreateMutex(
		NULL,
		FALSE,
		_T("FILE MUTEX"));

	if(!_mutex)
		return false;

	_slaveBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiSlave");

	_masterBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiMaster");


	WaitForSingleObject(_mutex,INFINITE);

	char* b = (char*) MapViewOfFile(_map,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		16);

	if(strcmp(kahawaiMaster,b)!=0)
	{
		strcpy(b,kahawaiMaster);
		_role = Master;
	}
	else
	{
		_role = Slave;
	}

	UnmapViewOfFile(b);
	ReleaseMutex(_mutex);

	return true;
}


DWORD Kahawai::CrossStreams(void)
{
	bool status = true;
	int length;

	byte* tmpFrame;
	int tmpSize;

	VERIFY(InitServerSocket());



	while(!_streamFinished)
	{

		if(_lastIFrameMerged%_iFps==0) 
		{
			EnterCriticalSection(&_iFrameLock);
			{
				//Wait while there is no I-Frame produced
				while(_iFrame==NULL)
					SleepConditionVariableCS(&_mergingCV,&_iFrameLock,INFINITE);
				tmpFrame = _iFrame;
				tmpSize = _iFrameSize;
				_iFrame = NULL;
				_iFrameSize  = -1;
			}
			LeaveCriticalSection(&_iFrameLock);
			WakeConditionVariable(&_encodingCV);

			VERIFY(InitSocketToClient());
			VERIFY(send(_clientSocket,(char*) tmpFrame,tmpSize,0)!=SOCKET_ERROR);

#ifdef WRITE_MOVIE
			KahawaiWriteFile("e:\\frames\\ip-test.h264",(char*) tmpFrame,tmpSize);
			KahawaiWriteFile("e:\\frames\\i-frames-%05d.h264",(char*) tmpFrame,tmpSize,_lastIFrameMerged);

#endif
			_lastIFrameMerged++;

		}
		else
		{
			//Here we receive from the loop socket the PPPP frames
			if(recv(_serverSocket,(char*)&length,4,0)==SOCKET_ERROR)
			{
				//On disconnection we assumed the stream is over
				//TODO:Of course it would be cleaner to send a special message
				_streamFinished = true;
				return 0;
			}

			int receivedBytes = 0;

			while(receivedBytes< length)
			{
				int burst = 0;
				burst = recv(_serverSocket,(char*)_PFrame,length,0);
				VERIFY(burst!=SOCKET_ERROR);
				int sent=0;

				VERIFY(InitSocketToClient());
				while(sent < burst)
				{
					sent+=send(_clientSocket,(char*) _PFrame,burst-sent,0);
				}
				receivedBytes+=burst;
			}
#ifdef WRITE_MOVIE		
			KahawaiWriteFile("e:\\frames\\ip-test.h264",(char*) _PFrame,length);
			KahawaiWriteFile("e:\\frames\\p-frames-%05d.h264",(char*) _PFrame,length,_lastIFrameMerged);
#endif
			_lastIFrameMerged++;
		}

	}

	return 0;
}

/********************************************************************************/
/* Initialization of Kahawai. Must be called before using other Kahawai services*/
/* Now seriously, the read config code is all shitty. Should be done right      */
/********************************************************************************/
bool Kahawai::Init()
{
	ifstream kahawaiiConfigFile;
	int server;
	int mode;
	string line;
	bool result = true;


	kahawaiiConfigFile.open(KAHAWAI_CONFIG,ios::in);
	if(!kahawaiiConfigFile.is_open())
	{
		return false;
	}

	//Read role configuration
	getline(kahawaiiConfigFile,line);
	server = atoi(line.c_str());

	if(!server==1)
	{
		_role = Client;
		InitializeFfmpeg(); //client will always need to decode
	}
	else //Otherwise is server, and can be either master or slave process. Determine through mapping
	{
		_role = Master;//for now counts as master. Effective role assigned on InitMapping
	}


	//Read role configuration
	getline(kahawaiiConfigFile,line);
	mode = atoi(line.c_str());

	switch(mode)
	{
	case DeltaEncoding:
		profile=DeltaEncoding;
		break;
	case IPFrame:
		//Read the frame rate
		getline(kahawaiiConfigFile,line);
		_iFps = atoi(line.c_str());

		profile=IPFrame;
		break;
	}


	if(IsClient())
	{
		getline(kahawaiiConfigFile,line);
		strcpy_s(_serverIP,line.c_str());
	}

	//read the configuration for the lo and hi resolution modes

	getline(kahawaiiConfigFile,line);
	_serverPort=atoi(line.c_str());

	getline(kahawaiiConfigFile,line);
	_loVideoWidth=atoi(line.c_str());

	getline(kahawaiiConfigFile,line);
	_loVideoHeight=atoi(line.c_str());

	getline(kahawaiiConfigFile,line);
	_hiVideoWidth=atoi(line.c_str());

	getline(kahawaiiConfigFile,line);
	_hiVideoHeight=atoi(line.c_str());


	//Initialize screen size related variables
	_hiFrameSize = (_hiVideoWidth * _hiVideoHeight * 3)/2; //YUV420p 4 bytes each 6 pixels
	//Rect used for SDL Player
	_screenRect.x = 0;
	_screenRect.y = 0;
	_screenRect.w = _hiVideoWidth;
	_screenRect.h = _hiVideoHeight;


	if(IsClient() && IsIFrame())
	{
		_timeStep = 1000 / _iFps;
		_iFrameTmp = new byte[_hiVideoHeight * _hiVideoWidth * 3]; //twice the size of the frame should be enough
		_PFrame = new byte[_hiVideoHeight * _hiVideoWidth * 3 * ((60 / _iFps) +1)]; //twice the size of the inter I-frames should be enough
	}
	else
	{
		_timeStep = 1000 / _fps;
	}


	if(IsServer() && !IsIFrame()) //IFrame encoding does not require a shared mapped file
	{
		kahawaiiConfigFile.close();
		//Initialize file mapping to share low resolution image between both processes
		result &= InitMapping((_loVideoHeight * _loVideoWidth * 3)/2); // (3/2) bits per pixel in YUV420p
	}

	result&=InitializeSynchronization();


	if(IsServer() || IsIFrame())
	{
		//Allocate the x264 picture structure
		if(!_x264_initialized)
			VERIFY(InitializeX264(_hiVideoWidth,_hiVideoHeight));

	}

	if(IsMaster() || IsIFrame())
	{
		//Not all 3 are used in all the scenarios.. so there is a little bit of memory wasted here
		_transformBuffer = new byte[3*_hiVideoWidth*_hiVideoHeight];
		_transformBufferTemp = new byte[3*_hiVideoWidth*_hiVideoHeight];
		_transformBufferTempCopy = new byte[3*_hiVideoWidth*_hiVideoHeight];
		_localVideoWidth = _hiVideoWidth;
		_localVideoHeight = _hiVideoHeight;
	}
	else
	{
		_transformBuffer = new byte[3*_hiVideoWidth*_hiVideoHeight];
		_transformBufferTemp = new byte[3*_hiVideoWidth*_hiVideoHeight];
		_transformBufferTempCopy = new byte[3*_loVideoWidth*_loVideoHeight];
		_localVideoWidth = _loVideoWidth;
		_localVideoHeight = _loVideoHeight;
	}



	x264_picture_alloc(_inputPicture, X264_CSP_I420, _hiVideoWidth, _hiVideoHeight);
	_inputPictureTemp = _inputPicture;

	kahawaiiConfigFile.close();
	return result;

}


VOID Kahawai::ReadFrameBuffer(int width, int height, byte *buffer) 
{
	glReadBuffer( GL_FRONT );
	glReadPixels( 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer ); 
}

/*******************************************************************************************/
/* Inits a socket that connects to the localhost server that will provide the IPPPPI stream*/
/*******************************************************************************************/
bool Kahawai::InitServerSocket()
{
	if(_serverSocketInitialized)
	{
		return true;
	}

	VERIFY(InitNetworking());

	//_serverSocketInitialized = true;

	//Initialize sockets and set any options
	int * p_int ;
	
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(_serverSocket == -1){
		//Error initializing socket
		return false;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if( (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(_serverSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
			//Error setting options
			free(p_int);
			return false;
	}
	free(p_int);

	//Connect to the server
	struct sockaddr_in my_addr;

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(_serverPort);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = inet_addr(_serverIP);


	if( connect( _serverSocket, (struct sockaddr*)&my_addr, sizeof(my_addr)) == SOCKET_ERROR ){
		DWORD putamadre = GetLastError();
		return false;
	}


	return true;

}

bool Kahawai::InitNetworking()
{
	if(_networkInitialized)
		return true;
#ifdef _MSC_VER
	//Initialize socket support WINDOWS ONLY!
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 2 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 || ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 )) {
			//"Could not find useable sock dll %d\n",WSAGetLastError());
			return false;
	}
#endif
	return true;
}



bool Kahawai::InitSocketToClient(){

	if(_socketToClientInitialized)
		return true;

	VERIFY(InitNetworking());

	_socketToClientInitialized = true;

	//The port you want the server to listen on
	int host_port= IsIFrame()&&IsClient()?_serverPort+1:_serverPort;

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

	while(true){

		if((_clientSocket = accept( hsock, (SOCKADDR*)&sadr, &addr_size))!= INVALID_SOCKET ){
			return true;
		}
		else
		{
			return false;
		}
	}
}


bool Kahawai::InitializeX264(int width, int height, int fps)
{
	x264_param_t param;


	if(IsDelta())
	{
		//define compression parameters
		x264_param_default_preset(&param, "superfast", "zerolatency");
		//For threading
		param.i_threads = 9;
		//		param.b_deterministic = 0; //This may be good enough in delta, dont know if it will work with i/P frame
		//		param.rc.i_lookahead = 0;
		//		param.i_sync_lookahead = 0;
		param.b_sliced_threads = 1;

		param.i_width = width;
		param.i_height = height;
		param.i_fps_num = fps;
		param.i_fps_den = 1;

		// Intra refres:
		param.i_keyint_max = fps;
		param.b_intra_refresh = 0;
		//Rate control:
		param.rc.i_rc_method = X264_RC_CRF;
		param.rc.f_rf_constant = 25;
		param.rc.f_rf_constant_max = 35;
		//For streaming:
		param.b_repeat_headers = 1;
		param.b_annexb = 1;

		x264_param_apply_profile(&param, "baseline");
		_x264_initialized = true;
		_encoder = x264_encoder_open(&param);
	}
	if(IsIFrame())
	{
			x264_param_default_preset(&param, "superfast", "zerolatency");
			param.i_threads = 9;
			//		param.b_deterministic = 0;
			//		param.rc.i_lookahead = 0;
			//		param.i_sync_lookahead = 0;
			param.b_sliced_threads = 1;

			param.i_width = width;
			param.i_height = height;
			param.i_fps_num = fps;
			param.i_fps_den = 1;
			// Intra refres:
			param.i_keyint_max = _iFps; //maximum iFps frames in between each iFrame
			param.i_scenecut_threshold = 0; //no scene_cut
			param.b_intra_refresh = 0;
			//Rate control:
			param.rc.i_rc_method = X264_RC_CRF;
			param.rc.f_rf_constant = 25;
			param.rc.f_rf_constant_max =35;
			//For streaming:
			param.b_repeat_headers = 1;
			param.b_annexb = 1;
			x264_param_apply_profile(&param, "baseline");
			_x264_initialized = true;
			_encoder = x264_encoder_open(&param);
	}

	return true;
}

/************************************************************************/
/* Initializes SDL to show video at the target high resolution          */
/************************************************************************/
bool Kahawai::InitializeSDL()
{
	if(_sdlInitialized)
		return true;

	//Initialize SDL Library
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		return false;
	}

	// Make a screen to put our video
	_pScreen = SDL_SetVideoMode(_hiVideoWidth, _hiVideoHeight, 0, 0);
	if(!_pScreen) {
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		exit(1);
	}

	// Allocate a place to put our YUV image on that screen
	_pYuvOverlay = SDL_CreateYUVOverlay(_hiVideoWidth,
		_hiVideoHeight,
		SDL_IYUV_OVERLAY,
		_pScreen);

	_sdlInitialized = true;

	return true;
}

bool Kahawai::LoadVideo(char* IP, int port, 
	AVFormatContext**	ppFormatCtx, 
	AVCodecContext**	ppCodecCtx, 
	AVFrame**			ppFrame)
{

	AVCodec*			pCodec;
	AVDictionary*		pCodec_opts = NULL;
	AVDictionary**		opts;

	*ppFormatCtx = avformat_alloc_context();
	(*ppFormatCtx)->flags |= AVFMT_FLAG_CUSTOM_IO;

	// Open video file
	char url[100];
	sprintf_s(url,100,"tcp://%s:%d",IP,port);

	if (avformat_open_input(ppFormatCtx, url, NULL, NULL) != 0)
		return false; // Couldn't open file

	opts = SetupFindStreamInfoOptions(*ppFormatCtx, pCodec_opts);
	// Retrieve stream information
	//if (avformat_find_stream_info(*ppFormatCtx, opts) < 0)
	//	return false; // Couldn't find stream information

	// Raw video. We only have one video stream (FIRST_VIDEO_STREAM)
	// If more streams have to be used. Find the right one iterating through _pFormatCtx->streams
	// Get a pointer to the codec context for the video stream
	*ppCodecCtx = (*ppFormatCtx)->streams[FIRST_VIDEO_STREAM]->codec;

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder((*ppCodecCtx)->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return false; // Codec not found
	}
	// Open codec
	if (avcodec_open2(*ppCodecCtx, pCodec, opts) < 0)
		return false; // Could not open codec

	// Allocate video frame
	*ppFrame = avcodec_alloc_frame();

	return true;

}

bool Kahawai::InitializeFfmpeg()
{

	if(_ffmpeg_initialized)
		return true;

	_ffmpeg_initialized = true;

	// Register all formats and codecs
	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	av_register_all();

	return true;
}

//DeltaEncoding Only
bool Kahawai::EncodeAndSend()
{
	x264_picture_t		pic_out;
	x264_nal_t*			nals;
	int					i_nals;

	//Encode the frame
	int frame_size = x264_encoder_encode(_encoder, &nals, &i_nals, _inputPicture, &pic_out);

	//write to TCP socket
	if (frame_size >= 0)
	{
		VERIFY(InitSocketToClient());
		VERIFY(send(_clientSocket,(char*) nals[0].p_payload,frame_size,0)!=SOCKET_ERROR);
#ifdef WRITE_MOVIE
		KahawaiWriteFile("e:\\frames-server\\deltaMovie.h264",(char*) nals[0].p_payload,frame_size);
#endif
	}

	return true;

}
//Iframe encoding Only (client)
bool Kahawai::EncodeIFrames()
{
	x264_picture_t		pic_out;
	x264_nal_t*			nals;
	int					i_nals;

	if(_inputPicture==NULL)
		return true;

	//Encode the frame
	_inputPicture->i_type = X264_TYPE_IDR; //lets try with an IDR frame first
	_inputPicture->i_qpplus1 = 1;
	int frame_size = x264_encoder_encode(_encoder, &nals, &i_nals, _inputPicture, &pic_out);
	if (frame_size >= 0)
	{
		memcpy(_iFrameTmp,nals[0].p_payload, frame_size);
		_iFrameTmpSize = frame_size;
	}

	return true;

}

//Iframe encoding only (server)
bool Kahawai::EncodeServerFrames()
{
	x264_picture_t		pic_out;
	x264_nal_t*			nals;
	int					i_nals;
#ifdef WRITE_MOVIE
	char name[100];
#endif 

	if(_renderedFrames%_iFps==0) //is it time to encode an I-Frame
	{
		_inputPicture->i_type = X264_TYPE_IDR; //lets try with an IDR frame first
		_inputPicture->i_qpplus1 = 1;
#ifdef WRITE_MOVIE
		sprintf_s(name,100,"e:\\frames-server\\si-frames-%05d.h264",_renderedFrames);
#endif
	}
	else
	{
		_inputPicture->i_type = X264_TYPE_P; //otherwise lets encode it as a P-Frame		
		_inputPicture->i_qpplus1 = X264_QP_AUTO;
#ifdef WRITE_MOVIE
		sprintf_s(name,100,"e:\\frames-server\\p-frames-%05d.h264",_renderedFrames);
#endif
	}

	//Encode the frame
	int frame_size = x264_encoder_encode(_encoder, &nals, &i_nals, _inputPicture, &pic_out);

	if(_renderedFrames%_iFps!=0) //We send only the P-frames
	{
		int sent = 0;
		VERIFY(InitSocketToClient());
		VERIFY(send(_clientSocket, (char*)&frame_size,sizeof(frame_size),0)!=SOCKET_ERROR); //advertise the frame size
		while(sent<frame_size)
		{
			sent += send(_clientSocket,((char*) nals[0].p_payload)+sent,frame_size-sent,0);
		}
	}
#ifdef WRITE_MOVIE
	KahawaiWriteFile(name,(char*)nals[0].p_payload, frame_size);
	KahawaiWriteFile("e:\\frames-server\\ip-test.h264",(char*)nals[0].p_payload, frame_size);
#endif
	return true;

}


//Transform the AVFrame into a SDL_Overlay
//Uses sws_scale in the process
void CopyFrameToOverlay(AVFrame* pFrame, SDL_Overlay* pOverlay)
{
	AVPicture pict;

	SDL_LockYUVOverlay(pOverlay);

	pict.data[0] = pOverlay->pixels[0];
	pict.data[1] = pOverlay->pixels[1];
	pict.data[2] = pOverlay->pixels[2];

	pict.linesize[0] = pOverlay->pitches[0];
	pict.linesize[1] = pOverlay->pitches[1];
	pict.linesize[2] = pOverlay->pitches[2];

	// Convert the image into YUV format that SDL uses
	struct SwsContext* img_convert_ctx =
		sws_getContext(pFrame->width, pFrame->height,  (PixelFormat)pFrame->format,
		pFrame->width, pFrame->height, PIX_FMT_YUV420P,
		SWS_FAST_BILINEAR, NULL, NULL, NULL);

	sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,
		0, pFrame->height, pict.data,
		pict.linesize);

	SDL_UnlockYUVOverlay(pOverlay);

}

bool Kahawai::DecodeAndShow(int width, int height,byte* low=NULL)
{
	int				frameFinished = 0;
	AVPacket		_Packet;
	static int			decodedFrame;

	InitializeFfmpeg();

	if(_streamFinished)
	{
		// Free the YUV frame
		//av_free(_pFrame);//TODO: Make sure to free this frame at some point.
		// Close the codec
		avcodec_close(_pCodecCtx);
		// Close the video file
		avformat_close_input(&_pFormatCtx);
		return false;
	}

	// Read frames 
	while(!frameFinished)
	{
		if (av_read_frame(_pFormatCtx, &_Packet) >= 0) 
		{
			// Is this a packet from the video stream?
			if (_Packet.stream_index == FIRST_VIDEO_STREAM) 
			{
				// Decode video frame
				VERIFY(avcodec_decode_video2(_pCodecCtx, _pAVFrame, &frameFinished, &_Packet)>0);
				// Did we get a video frame?
				if (frameFinished) 
				{
					CopyFrameToOverlay(_pAVFrame,_pYuvOverlay);

					//Patch the frame
					if(IsDelta())
					{
						for (int i=0 ; i< _hiFrameSize ; i++) 
						{
							_pYuvOverlay->pixels[0][i] = Patch(_pYuvOverlay->pixels[0][i],low[i]);
						}
					}
#ifdef WRITE_DECODED_FRAMES
					//writes the raw yuv file to disc as well
					KahawaiWriteFile("e:\\frames\\yuv\\frame%04d.yuv",(char*)_pYuvOverlay->pixels[0],(width*height*3)/2,decodedFrame);
#endif 
					SDL_DisplayYUVOverlay(_pYuvOverlay, &_screenRect);
					decodedFrame++;
				}
			}

			// Free the packet that was allocated by av_read_frame
			av_free_packet(&_Packet);
			//No event Polling. 
			//When using with input forwarding to the server, add input handling loop (SDL_PollEvent)
		}
		else
		{
			frameFinished = 1;
			_streamFinished = true;
		}
	}


	return true;
}



bool Kahawai::DecodeAndMix(int width, int height)
{
	int				frameFinished = 0;
	AVPacket		_Packet;

	if(_streamFinished)
	{
		// Free the YUV frame
		//av_free(_pFrame);//TODO: Make sure to free this frame at some point.
		// Close the codec
		avcodec_close(_pCodecCtx);
		// Close the video file
		avformat_close_input(&_pFormatCtx);
		return false;
	}

	// Read frames 
	while(!frameFinished)
	{
		if (av_read_frame(_pFormatCtx, &_Packet) >= 0) 
		{
			// Is this a packet from the video stream?
			if (_Packet.stream_index == FIRST_VIDEO_STREAM) 
			{
				// Decode video frame
				avcodec_decode_video2(_pCodecCtx, _pAVFrame, &frameFinished, &_Packet);
				// Did we get a video frame?
				if (frameFinished) 
				{
					CopyFrameToOverlay(_pAVFrame,_pYuvOverlay);

					//Patch the frame
					for (int i=0 ; i< _hiFrameSize ; i++) 
					{
						//_pYuvOverlay->pixels[0][i] = Patch(_pYuvOverlay->pixels[0][i],low[i]);
					}

					SDL_DisplayYUVOverlay(_pYuvOverlay, &_screenRect);
				}
			}

			// Free the packet that was allocated by av_read_frame
			av_free_packet(&_Packet);
			//No event Polling. 
			//When using with input forwarding to the server, add input handling loop (SDL_PollEvent)
		}
		else
		{
			frameFinished = 1;
			_streamFinished = true;
		}
	}


	return true;

}

void Kahawai::MapRegion()
{
	if(_role==Master)
	{
		//while the slave will write the low detail version to a memory mapped file
		_mappedBuffer = (byte*) MapViewOfFile(_map,
			FILE_MAP_READ,
			0,
			0,
			(_loVideoWidth*_loVideoHeight*3)/2);

	}
	if(_role==Slave)
	{
		//while the slave will write the low detail version to a memory mapped file
		_mappedBuffer = (byte*) MapViewOfFile(_map,
			FILE_MAP_WRITE,
			0,
			0,
			(_loVideoWidth*_loVideoHeight*3)/2);
	}
}


//////////////////////////////////////////////////////////////////////////
// Async Handlers
//////////////////////////////////////////////////////////////////////////
DWORD WINAPI Kahawai::DeltaEncodeAsync( void* Param) 
{
	Kahawai* This = (Kahawai*) Param;	


	while(This->_offloading)
	{
		This->DeltaEncode(This->_localVideoWidth,This->_localVideoHeight);
		This->_frameInPipeLine++;
	}

	return 0;
}

DWORD WINAPI Kahawai::IFrameEncodeAsync(void* Param) 
{
	Kahawai* This = (Kahawai*) Param;	


	while(This->_offloading)
	{
		This->IFrameEncode(This->_localVideoWidth,This->_localVideoHeight);
		This->_frameInPipeLine++;
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////
// Main techniques entry points
//////////////////////////////////////////////////////////////////////////


/*
================== 
DeltaEncode
================== 
*/  
void Kahawai::DeltaEncode( int width, int height) {

	int					pix				= width * height;
	struct SwsContext*	convertCtx;

	//2. Synchronize both processes if running at the server
	if(_role==Master)
	{
		//signal and wait for slave
		SetEvent(_masterBarrier);
		WaitForSingleObject(_slaveBarrier, INFINITE);
	}
	if(_role==Slave)
	{
		//signal and wait for master
		SetEvent(_slaveBarrier);
		WaitForSingleObject(_masterBarrier,INFINITE);
	}

	//3. Get Memory Mapped Region
	if(IsServer() && _mappedBuffer==NULL)
		MapRegion();

	//4. Transformed (and scale) captured screen to YUV420p at target high resolution
	//Libswscale flips the image when converting it. Flip it in advance, to later get the correct converted image

	//Synchronization
	EnterCriticalSection(&_captureLock);
	{
		while(_transformBuffer == NULL)
		{
			SleepConditionVariableCS(&_encodingCV,&_captureLock,INFINITE);
		}

		VerticalFlip(width,height, _transformBuffer,3);

		int srcstride = width*3; //RGB stride is just 3*width

		//Allocate the x264 picture structure
		if(!_x264_initialized)
			InitializeX264(_hiVideoWidth,_hiVideoHeight);


		//Convert the image to YUV420
		if(_role==Master)
		{
			convertCtx = sws_getContext(width, height, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
		}
		else
		{	//TODO: Need to validate wheter bilinear against bicubic is worth it. (Specially at the client)
			convertCtx = sws_getContext(_loVideoWidth, _loVideoHeight, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
		}

		uint8_t *src[3]= {_transformBuffer, NULL, NULL}; 
		sws_scale(convertCtx, src, &srcstride, 0, height, _inputPicture->img.plane, _inputPicture->img.i_stride);
		_transformBuffer = NULL;

#ifdef WRITE_SOURCE_FRAME
		if(IsMaster())
			KahawaiWriteFile("e:\\frames-server\\yuv\\frame%04d.yuv",(char*)_inputPicture->img.plane[0],(width*height*3)/2,_renderedFrames);
#endif

	}
	LeaveCriticalSection(&_captureLock);
	WakeConditionVariable(&_capturingCV);

	//5. Role specific 
	switch(_role)
	{
	case Master:
		//Apply delta to each byte of the image
		for (int i=0 ; i<_hiFrameSize ; i++) 
		{
			_inputPicture->img.plane[0][i] = Delta(_inputPicture->img.plane[0][i],_mappedBuffer[i]);
		}
		EncodeAndSend();
		break;
	case Slave:
		//write to mapped file
		memcpy(_mappedBuffer,_inputPicture->img.plane[0],(width*height*3)/2);
		break;
	case Client:
		VERIFY(InitializeFfmpeg());
		VERIFY(InitializeSDL());
		if(!_loadedDeltaStream)
		{
			LoadVideo(_serverIP,_serverPort,&_pFormatCtx,&_pCodecCtx,&_pAVFrame);
			_loadedDeltaStream = true;
		}

		DecodeAndShow(width, height,_inputPicture->img.plane[0]);
		break;
	}

	//6. Cleanup
	if(_role!=Client)
	{
		sws_freeContext(convertCtx);
	}


}

/*
================== 
IFrameEncode
================== 
*/  

void Kahawai::IFrameEncode( int width, int height) {

	int					pix				= width * height;
	struct SwsContext*	convertCtx;


	if(IsMaster() || _renderedFrames % _iFps == 0)
	{
		//1. Capture screen to buffer
		ReadFrameBuffer( width, height, _transformBuffer);


		//2. Transformed (and scale) captured screen to YUV420p
		//Libswscale flips the image when converting it. Flip it in advance, to later get the correct converted image
		VerticalFlip(width,height, _transformBuffer,3);

		int srcstride = width*3; //RGB stride is just 3*width

		_inputPicture = _inputPictureTemp;

		//Convert the image to YUV420
		convertCtx = sws_getContext(width, height, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

		uint8_t *src[3]= {_transformBuffer, NULL, NULL}; 
		sws_scale(convertCtx, src, &srcstride, 0, height, _inputPicture->img.plane, _inputPicture->img.i_stride);

#ifdef WRITE_SOURCE_FRAME
		KahawaiWriteFile("e:\\frames-server\\yuv\\frame%04d.yuv",(char*)_inputPicture->img.plane[0],(width*height*3)/2,_renderedFrames);
#endif

	}

	//3. Role specific 
	switch(_role)
	{
	case Master:
		EncodeServerFrames();
		break;
	case Client:
		if(!_crossStreamInitialized)
			_crossStreamInitialized = CreateKahawaiThread(CrossStreamsThreadStart,this); //Set up the mutex and threads to render and decode
		//encode all as I-Frames send them to localhost socket

		VERIFY(EncodeIFrames());

		EnterCriticalSection(&_iFrameLock);
		{
			while(_iFrame!=NULL)
			{
				SleepConditionVariableCS(&_encodingCV,&_iFrameLock, INFINITE);
			}
			if(_inputPicture!=NULL)
			{
				_iFrame = _iFrameTmp;
				_iFrameSize = _iFrameTmpSize;
				_inputPicture = NULL;

			}

		}
		LeaveCriticalSection(&_iFrameLock);
		WakeConditionVariable(&_mergingCV);

		//The following initialization only happens once
		InitializeSDL();
		if(!_loadedDeltaStream)
		{
			LoadVideo("localhost",_serverPort+1,&_pFormatCtx,&_pCodecCtx,&_pAVFrame);
			_loadedDeltaStream = true;
		}

		if(!_decodeThreadInitialized)
			_decodeThreadInitialized = CreateKahawaiThread(DecodeThreadStart,this);

		break;

	}


	//4. Cleanup
	if(_role!=Client)
	{
		sws_freeContext(convertCtx);
	}


}




//////////////////////////////////////////////////////////////////////////
// Offload Public Interface
//////////////////////////////////////////////////////////////////////////
void Kahawai::StartOffload()
{
	_offloading = true;
	_renderedFrames = 0;
	DWORD ThreadID;
	HANDLE thread;

	switch(profile)
	{
	case DeltaEncoding:
		CreateKahawaiThread(DeltaEncodeAsync,this);
		break;
	case IPFrame:
//		if(IsServer())
//			CreateKahawaiThread(IFrameEncodeAsync,this);
		break;
	}
}

void Kahawai::StopOffload()
{
	if(!_offloading)
		return;

	_offloading = false;

}

void Kahawai::LogFPS()
{
	offloadEndTime = timeGetTime();
	DWORD totalTime = offloadEndTime - offloadStartTime;
	double fps = ((_renderedFrames - 147)*1000) / totalTime;
	char result[100];
	int size = sprintf(result,"Average FPS:%f\nTotal Frames:%d\nTime elapsed:%d\r\t",fps,_renderedFrames-147,totalTime);

	KahawaiWriteFile("KahawaiFPS.log", result,size);

}

void Kahawai::ReadCurrentFrame()
{
	ReadFrameBuffer( _localVideoWidth, _localVideoHeight, _transformBufferTemp);
	EnterCriticalSection(&_captureLock);
	{
		while(_transformBuffer!=NULL)
		{
			SleepConditionVariableCS(&_capturingCV,&_captureLock,INFINITE);
		}
		
		_transformBuffer = _transformBufferTempCopy;
		memcpy(_transformBuffer,_transformBufferTemp,_localVideoWidth*_loVideoHeight*3);	
	}
	LeaveCriticalSection(&_captureLock);
	WakeConditionVariable(&_encodingCV);
}


void Kahawai::OffloadVideo(	 int width, int height)
{
	if(!_offloading)
		_offloading = true;

	if(_renderedFrames == 100)
	{
		offloadStartTime = timeGetTime();
	}

	if(_renderedFrames == 600)
	{
		exit(0);
		LogFPS();
	}


	switch(profile)
	{
	case DeltaEncoding:
		ReadCurrentFrame();
		break;
	case IPFrame:
//		if(IsMaster())
//			ReadCurrentFrame();
//		else
			IFrameEncode(width,height);
		break;
	}

	_renderedFrames++;

	return;
}



//////////////////////////////////////////////////////////////////////////
// Time Control
//////////////////////////////////////////////////////////////////////////

int Kahawai::Sys_Milliseconds( void ) {
	int sys_curtime;
	if ( !_timeInitialized ) {
		_sys_timeBase = timeGetTime();
		_sys_prevFrame = _renderedFrames; //CUERVO:THis may be a synchronization issue
		_sys_prevTime = _sys_timeBase;
		_timeInitialized = true;
	}
	int frame = _renderedFrames;

	if(frame > _sys_prevFrame)
	{
		sys_curtime = _sys_prevTime + ((frame - _sys_prevFrame) * _timeStep);
		_sys_prevFrame = frame;
		_sys_prevTime = sys_curtime;
	}
	else
	{
		sys_curtime = _sys_prevTime;
	}

	return sys_curtime; 
}


bool Kahawai::ShouldSkipFrame()
{
	if(IsServer())
		return false;

	if(IsDelta())
		return false;

	if(IsIFrame())
		return (_renderedFrames % _iFps!=0);

	return false;
}

Kahawai kahawai;

#endif