#include<iostream>

using namespace std;

#ifdef KAHAWAI
#include "Kahawai.h"

#include<fstream>
#include<string>

/************************************************************************/
/* General Kahawai Utilities                                            */ 
/************************************************************************/

//////////////////////////////////////////////////////////////////////////
// Basic Delta encoding transformation
//////////////////////////////////////////////////////////////////////////
byte Delta(byte hi, byte lo)
{
	int resultPixel = ((hi - lo) / 2) + 127;

	if (resultPixel > 255)
		resultPixel = 255;

	if (resultPixel < 0)
		resultPixel = 0;

	return (byte)resultPixel;
}

//////////////////////////////////////////////////////////////////////////
// Basic Patch decoding transformation
//////////////////////////////////////////////////////////////////////////
byte Patch(byte delta, byte lo)
{
	int resultPixel = (2 * (delta - 127)) + lo;

	if (resultPixel > 255)
		resultPixel = 255;

	if (resultPixel < 0)
		resultPixel = 0;

	return (byte)resultPixel;
}

//////////////////////////////////////////////////////////////////////////
// Flips vertically a bitmap
//////////////////////////////////////////////////////////////////////////
void VerticalFlip(int width, int height, byte* pixelData, int bitsPerPixel)
{
	byte* temp = new byte[width*bitsPerPixel];
	height--; //remember height array ends at height-1


	for (int y = 0; y < (height+1)/2; y++) 
	{
		memcpy(temp,&pixelData[y*width*bitsPerPixel],width*bitsPerPixel);
		memcpy(&pixelData[y*width*bitsPerPixel],&pixelData[(height-y)*width*bitsPerPixel],width*bitsPerPixel);
		memcpy(&pixelData[(height-y)*width*bitsPerPixel],temp,width*bitsPerPixel);
	}		
}


/************************************************************************/
/* Kahawai Class Methods                                                */
/************************************************************************/
Kahawai::Kahawai(void)
{
	_x264_initialized = false;
	_encoder = NULL;
	_ffmpeg_initialized = false;
	_networkInitialized = false;
	_timeInitialized = false;
	_mappedBuffer = NULL;
	_decodeThreadInitialized = false;
	_renderedFrames = 0;
	_fps=60;
	_offloading = false;
	_decodedServerFrames = 0;


}


Kahawai::~Kahawai(void)
{
}


bool Kahawai::InitializeIFrameSharing()
{

	HANDLE iFrameMutex = CreateMutex(
		NULL,
		TRUE,
		_T("IFrameMutex"));

	if(!iFrameMutex)
		return false;
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
		_iFrameBuffer = new byte[_loVideoHeight * _loVideoWidth * 3]; //twice the size of the frame should be enough
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
	else
	{
		if(IsIFrame())
		{
			result&=InitializeIFrameSharing();
		}
	}

	kahawaiiConfigFile.close();
	return result;

}


VOID Kahawai::ReadFrameBuffer(int width, int height, byte *buffer) 
{
	glReadBuffer( GL_FRONT );
	glReadPixels( 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer ); 
}


int Kahawai::InitNetwork(){

	if(_networkInitialized)
		return true;

	_networkInitialized = true;

	//The port you want the server to listen on
	int host_port= _serverPort;

	//Initialize socket support WINDOWS ONLY!
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 2 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 || ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 )) {
			//"Could not find useable sock dll %d\n",WSAGetLastError());
			return -1;
	}

	//Initialize sockets and set any options
	int hsock;
	int * p_int ;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		//"Error initializing socket %d\n",WSAGetLastError());
		return -1;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
			//"Error setting options %d\n", WSAGetLastError());
			free(p_int);
			return -1;
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
		return -1;
	}
	if(listen( hsock, 10) == -1 ){
		//"Error listening %d\n",WSAGetLastError());
		return -1;
	}	

	//Now lets to the server stuff

	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);

	while(true){

		if((_socket = accept( hsock, (SOCKADDR*)&sadr, &addr_size))!= INVALID_SOCKET ){
			return 0;
		}
		else
		{
			return -1;
		}
	}
}


bool Kahawai::InitializeX264(int width, int height, int fps)
{
	x264_param_t param;


	if(IsDelta())
	{
		//define compression parameters
		x264_param_default_preset(&param, "veryfast", "zerolatency");
		param.i_threads = 1;
		param.b_deterministic = 1; //useful when multithreading encoding
		param.i_width = width;
		param.i_height = height;
		param.i_fps_num = fps;
		param.i_fps_den = 1;
		// Intra refres:
		param.i_keyint_max = fps;
		param.b_intra_refresh = 1;
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
		if(IsClient())
		{
			x264_param_default_preset(&param, "placebo", "zerolatency");
			param.i_threads = 1;
			param.b_deterministic = 1; //useful when multithreading encoding
			param.i_width = width;
			param.i_height = height;
			param.i_fps_num = _iFps; //client only renders iFrames at iFps frames per second
			param.i_fps_den = 1;
			// Intra refresh:
			param.i_keyint_max = 1;
			param.i_scenecut_threshold = 0; //no scene_cut
			param.b_intra_refresh = 1;
			//Rate control:
			param.rc.i_rc_method = X264_RC_CQP; //Constant quality (ALMOST no compression)
			param.rc.i_qp_constant = 0;
			param.rc.i_qp_max = 0;
			param.rc.i_qp_min = 0;
			//For streaming:
			param.b_repeat_headers = 1;
			param.b_annexb = 1;
			x264_param_apply_profile(&param, "baseline");
			_x264_initialized = true;
			_encoder = x264_encoder_open(&param);
		}
		else //is Server
		{
			x264_param_default_preset(&param, "placebo", "zerolatency");
			param.i_threads = 1;
			param.b_deterministic = 1; //useful when multithreading encoding
			param.i_width = width;
			param.i_height = height;
			param.i_fps_num = fps;
			param.i_fps_den = 1;
			// Intra refres:
			param.i_keyint_max = _iFps; //maximum iFps frames in between each iFrame
			param.i_scenecut_threshold = 0; //no scene_cut
			param.b_intra_refresh = 1;
			//Rate control:
			param.rc.i_rc_method = X264_RC_CQP;
			param.rc.i_qp_constant = 0;
			param.rc.i_qp_max = 25;
			param.rc.i_qp_min = 0;
			param.rc.i_qp_step = 25;
			//For streaming:
			param.b_repeat_headers = 1;
			param.b_annexb = 1;
			x264_param_apply_profile(&param, "baseline");
			_x264_initialized = true;
			_encoder = x264_encoder_open(&param);
		}
	}

	return true;
}

bool Kahawai::InitializeFfmpeg()
{
	if(_ffmpeg_initialized)
		return 0;

	_ffmpeg_initialized = true;

	// Register all formats and codecs
	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	av_register_all();

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		return false;
	}

	_pFormatCtx = avformat_alloc_context();
	_pFormatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

	// Open video file
	char url[100];
	sprintf_s(url,100,"tcp://%s:%d",_serverIP,_serverPort);

	if (avformat_open_input(&_pFormatCtx, url, NULL, NULL) != 0)
		return false; // Couldn't open file

	opts = SetupFindStreamInfoOptions(_pFormatCtx, _pCodec_opts);
	// Retrieve stream information
	if (avformat_find_stream_info(_pFormatCtx, opts) < 0)
		return false; // Couldn't find stream information

	// Find the first video stream
	_pVideoStream = -1;
	for (unsigned int i = 0; i < _pFormatCtx->nb_streams; i++)
		if (_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			_pVideoStream = i;
			break;
		}
		if (_pVideoStream == -1)
			return false; // Didn't find a video stream

		// Get a pointer to the codec context for the video stream
		_pCodecCtx = _pFormatCtx->streams[_pVideoStream]->codec;

		// Find the decoder for the video stream
		_pCodec = avcodec_find_decoder(_pCodecCtx->codec_id);
		if (_pCodec == NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return false; // Codec not found
		}
		// Open codec
		if (avcodec_open2(_pCodecCtx, _pCodec, opts) < 0)
			return false; // Could not open codec

		// Allocate video frame
		_pFrame = avcodec_alloc_frame();

		// Make a screen to put our video
		_pScreen = SDL_SetVideoMode(_pCodecCtx->width, _pCodecCtx->height, 0, 0);
		if(!_pScreen) {
			fprintf(stderr, "SDL: could not set video mode - exiting\n");
			exit(1);
		}

		// Allocate a place to put our YUV image on that screen
		_pBmp = SDL_CreateYUVOverlay(_pCodecCtx->width,
			_pCodecCtx->height,
			SDL_IYUV_OVERLAY,
			_pScreen);


		return true;
}

bool Kahawai::EncodeIFrames(x264_picture_t* pic_in)
{
	x264_picture_t		pic_out;
	x264_nal_t*			nals;
	int					i_nals;

#ifdef WRITE_CAPTURE
	fileSystem->WriteFile( fileName, pic_in.img.plane[0], c );
#endif


	//Encode the frame
	int frame_size = x264_encoder_encode(_encoder, &nals, &i_nals, pic_in, &pic_out);

//#ifdef WRITE_IFRAME
	if (frame_size >= 0)
	{
		ofstream movieOut ("i-frames.h264", ios::out | ios::binary | ios::app);
		movieOut.write((char*)nals[0].p_payload, frame_size);
		movieOut.close();
	}
//#endif

	//Copy the I-Frame data to the buffer
	memcpy(_iFrameBuffer,(char*)nals[0].p_payload,frame_size);
	
	return true;

}


bool Kahawai::EncodeAndSend(x264_picture_t* pic_in)
{
	int					i, c;
	x264_picture_t		pic_out;
	x264_nal_t*			nals;
	int					i_nals;

	#ifdef WRITE_CAPTURE
	fileSystem->WriteFile( fileName, pic_in.img.plane[0], c );
	#endif

	c = (_hiVideoWidth * _hiVideoHeight * 3)/2; //YUV420p 4 bytes each 6 pixels

	//Apply delta to each byte of the image
	for (i=0 ; i<c ; i+=3) {
		pic_in->img.plane[0][i] = Delta(pic_in->img.plane[0][i],_mappedBuffer[i]);
		pic_in->img.plane[0][i+1] = Delta(pic_in->img.plane[0][i+1],_mappedBuffer[i+1]);
		pic_in->img.plane[0][i+2] = Delta(pic_in->img.plane[0][i+2],_mappedBuffer[i+2]);
	}


#ifdef WRITE_DELTA
	fileSystem->WriteFile( fileName, pic_in.img.plane[0], c );
#endif

	//Encode the frame
	int frame_size = x264_encoder_encode(_encoder, &nals, &i_nals, pic_in, &pic_out);

	#ifdef WRITE_VIDEO		
	if (frame_size >= 0)
	{
		idFile* movieOut;
		movieOut = fileSystem->OpenFileAppend("kahawai.h264");

		//fileSystem->WriteFile( fileName, pic_out.img.plane[0], frame_size );
		movieOut->Write(nals[0].p_payload, frame_size);
		fileSystem->CloseFile(movieOut);

	}
	#endif

	//write to TCP socket
	if (frame_size >= 0)
	{
		assert(InitNetwork());
		send(_socket,(char*) nals[0].p_payload,frame_size,0);
	}

	return true;

}

//Transform the AVFrame into a SDL_Overlay
//Uses sws_scale in the process
void CopyFrameToOverlay(AVFrame* pFrame, SDL_Overlay* pOverlay, byte* low)
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

int Kahawai::DecodeAndShow(byte* low,int width, int height)
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
		av_close_input_file(_pFormatCtx);
		return -1;
	}

	// Read frames 
	while(!frameFinished)
	{
		if (av_read_frame(_pFormatCtx, &_Packet) >= 0) 
		{
			// Is this a packet from the video stream?
			if (_Packet.stream_index == _pVideoStream) 
			{
				// Decode video frame
				avcodec_decode_video2(_pCodecCtx, _pFrame, &frameFinished, &_Packet);
				// Did we get a video frame?
				if (frameFinished) 
				{
					CopyFrameToOverlay(_pFrame,_pBmp,low);

					//Patch the frame
					for (int i=0 ; i< _hiFrameSize ; i++) 
					{
						_pBmp->pixels[0][i] = Patch(_pBmp->pixels[0][i],low[i]);
					}

					SDL_DisplayYUVOverlay(_pBmp, &_screenRect);
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


	return 0;
}

int Kahawai::DecodeAndMix(int width, int height)
{
	/*
	int frameFinished = 0;

	//Mutex to wait for renderer to finish I-Frame
	HANDLE iFrameMutex = OpenMutex(
		SYNCHRONIZE,
		FALSE,
		_T("IFrameMutex")
		);

	if(_streamFinished)
		return -1;

	// Read frames 
	while(!frameFinished)
	{
		//Next frame is an I-Frame rendered locally
		if(_decodedServerFrames % _iFps == 0)
		{
			//Wait for rendering thread to finish
			WaitForSingleObject(iFrameMutex,INFINITE);
			_decodedServerFrames++;

			//now should do the whole
			av_read_frame(_pFormatCtx, (AVPacket *)_iFrameBuffer);
		}
		else
		{

			if (av_read_frame(_pFormatCtx, &_Packet) >= 0) 
			{
				// Is this a packet from the video stream?
				if (_Packet.stream_index == _pVideoStream) 
				{
					_decodedServerFrames++;
					// Decode video frame
					avcodec_decode_video2(_pCodecCtx, _pFrame, &frameFinished, &_Packet);
					// Did we get a video frame?
					if (frameFinished) 
					{
						SDL_LockYUVOverlay(_pBmp);

						AVPicture pict;
						pict.data[0] = _pBmp->pixels[0];
						pict.data[1] = _pBmp->pixels[1];
						pict.data[2] = _pBmp->pixels[2];

						pict.linesize[0] = _pBmp->pitches[0];
						pict.linesize[1] = _pBmp->pitches[1];
						pict.linesize[2] = _pBmp->pitches[2];

						// Convert the image into YUV format that SDL uses
						struct SwsContext* img_convert_ctx =
							sws_getContext(_pCodecCtx->width, _pCodecCtx->height,_pCodecCtx->pix_fmt,
							_pCodecCtx->width, _pCodecCtx->height, PIX_FMT_YUV420P,
							SWS_BICUBIC, NULL, NULL, NULL);

						sws_scale(img_convert_ctx, _pFrame->data, _pFrame->linesize,
							0, _pCodecCtx->height, pict.data,
							pict.linesize);

						SDL_UnlockYUVOverlay(_pBmp);

						_Rect.x = 0;
						_Rect.y = 0;
						_Rect.w = _pCodecCtx->width;
						_Rect.h = _pCodecCtx->height;
						SDL_DisplayYUVOverlay(_pBmp, &_Rect);
					}
				}

				// Free the packet that was allocated by av_read_frame
				av_free_packet(&_Packet);
				SDL_PollEvent(&_Event);
				switch(_Event.type) 
				{
				case SDL_QUIT:
					SDL_Quit();
					exit(0);
					break;
				default:
					break;
				}

			}
			else
			{
				// Free the YUV frame
				av_free(_pFrame);

				// Close the codec
				avcodec_close(_pCodecCtx);

				// Close the video file
				av_close_input_file(_pFormatCtx);
				frameFinished = 1;
				_streamFinished = true;
			}

		}
	}
	*/
	return 0;
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

/*
================== 
CaptureDelta
================== 
*/  
void Kahawai::CaptureDelta( int width, int height, int frameNumber) {

	int					pix				= width * height;
	byte*				buffer			= new byte[3*pix];
	struct SwsContext*	convertCtx;
	x264_picture_t		pic_in;

	//1. Capture screen to buffer
	ReadFrameBuffer( width, height, buffer);

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
	VerticalFlip(width,height, buffer,3);

	int srcstride = width*3; //RGB stride is just 3*width

	//Allocate the x264 picture structure
	if(!_x264_initialized)
		InitializeX264(_hiVideoWidth,_hiVideoHeight);

	x264_picture_alloc(&pic_in, X264_CSP_I420, _hiVideoWidth, _hiVideoHeight);

	//Convert the image to YUV420
	if(_role==Master)
	{
		convertCtx = sws_getContext(width, height, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	}
	else
	{	//TODO: Need to validate wheter bilinear against bicubic is worth it. (Specially at the client)
		convertCtx = sws_getContext(_loVideoWidth, _loVideoHeight, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	}

	uint8_t *src[3]= {buffer, NULL, NULL}; 
	sws_scale(convertCtx, src, &srcstride, 0, height, pic_in.img.plane, pic_in.img.i_stride);

	//5. Role specific 
	switch(_role)
	{
	case Master:
		EncodeAndSend(&pic_in);
		break;
	case Slave:
		//write to mapped file
		memcpy(_mappedBuffer,pic_in.img.plane[0],(width*height*3)/2);
		break;
	case Client:
		assert(InitializeFfmpeg());
		DecodeAndShow(pic_in.img.plane[0], width, height);
	}

	//6. Cleanup
	if(_role!=Client)
	{
		sws_freeContext(convertCtx);
	}

	x264_picture_clean(&pic_in);
	delete buffer;

}

void Kahawai::CaptureIFrame( int width, int height, int frameNumber) {

	int					pix				= width * height;
	byte*				buffer			= new byte[3*pix];
	struct SwsContext*	convertCtx;
	x264_picture_t		pic_in;

	//1. Capture screen to buffer
	ReadFrameBuffer( width, height, buffer);


	//2. Transformed (and scale) captured screen to YUV420p
	//Libswscale flips the image when converting it. Flip it in advance, to later get the correct converted image
	VerticalFlip(width,height, buffer,3);

	int srcstride = width*3; //RGB stride is just 3*width

	//Allocate the x264 picture structure
	if(!_x264_initialized)
		InitializeX264(_hiVideoWidth,_hiVideoHeight);

	x264_picture_alloc(&pic_in, X264_CSP_I420, _hiVideoWidth, _hiVideoHeight);

	//Convert the image to YUV420
	convertCtx = sws_getContext(width, height, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

	uint8_t *src[3]= {buffer, NULL, NULL}; 
	sws_scale(convertCtx, src, &srcstride, 0, height, pic_in.img.plane, pic_in.img.i_stride);

	//3. Role specific 
	switch(_role)
	{
	case Master:
		//encode all sequence, strip I-Frames, send
		break;
	case Client:
		//encode all as I-Frames, notify display thread when done
		EncodeIFrames(&pic_in);
		break;

	}

	//4. Cleanup
	if(_role!=Client)
	{
		sws_freeContext(convertCtx);
	}

	x264_picture_clean(&pic_in);
	delete buffer;

}


//Public Accessors

bool Kahawai::IsMaster()
{
	return _role == Master;
}

bool Kahawai::IsSlave()
{
	return _role == Slave;
}

bool Kahawai::IsClient()
{
	return _role == Client;
}

bool Kahawai::IsServer()
{
	return _role != Client;
}

KAHAWAI_MODE Kahawai::GetRole()
{
	return _role;
}

bool Kahawai::IsDelta()
{
	return profile == DeltaEncoding;
}

bool Kahawai::IsIFrame()
{
	return profile == IPFrame;
}

ENCODING_PROFILE Kahawai::GetMode()
{
	return profile;
}

bool Kahawai::IsOffloading()
{
	return _offloading;
}


AVDictionary* Kahawai::FilterCodecOptions(AVDictionary *opts, enum CodecID codec_id,
	int encoder) {
		AVDictionary *ret = NULL;
		AVDictionaryEntry *t = NULL;
		AVCodec *codec =
			encoder ?
			avcodec_find_encoder(codec_id) :
		avcodec_find_decoder(codec_id);
		int flags =
			encoder ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
		char prefix = 0;

		if (!codec)
			return NULL;

		switch (codec->type) {
		case AVMEDIA_TYPE_VIDEO:
			prefix = 'v';
			flags |= AV_OPT_FLAG_VIDEO_PARAM;
			break;
		case AVMEDIA_TYPE_AUDIO:
			prefix = 'a';
			flags |= AV_OPT_FLAG_AUDIO_PARAM;
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			prefix = 's';
			flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
			break;
		default:
			break;
		}

		while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) {
			if (av_opt_find(_avcodec_opts[0], t->key, NULL, flags, 0)
				|| (codec && codec->priv_class
				&& av_opt_find(&codec->priv_class, t->key, NULL, flags,
				0)))
				av_dict_set(&ret, t->key, t->value, 0);
			else if (t->key[0] == prefix
				&& av_opt_find(_avcodec_opts[0], t->key + 1, NULL, flags, 0))
				av_dict_set(&ret, t->key + 1, t->value, 0);
		}
		return ret;
}

AVDictionary** Kahawai::SetupFindStreamInfoOptions(AVFormatContext *s,
	AVDictionary *codec_opts) {
		AVDictionary **opts;

		if (!s->nb_streams)
			return NULL;
		opts = (AVDictionary**)av_mallocz(s->nb_streams * sizeof(*opts));
		if (!opts) {
			av_log(NULL, AV_LOG_ERROR,
				"Could not alloc memory for stream options.\n");
			return NULL;
		}
		for (unsigned int i = 0; i < s->nb_streams; i++)
			opts[i] = FilterCodecOptions(codec_opts, s->streams[i]->codec->codec_id,
			0);
		return opts;
}


void Kahawai::OffloadVideo(	 int width, int height, int frameNumber)
{
	_offloading = true;
	switch(profile)
	{
	case DeltaEncoding:
		CaptureDelta(width,height,frameNumber);
		break;
	case IPFrame:
		CaptureIFrame(width,height,frameNumber);
		break;
	}
	_renderedFrames++;
	return;
}

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

Kahawai kahawai;
#endif