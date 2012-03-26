#include "Kahawai.h"

#include<iostream>
#include<fstream>
#include<string>

using namespace std;

Kahawai::Kahawai(void)
{
	x264_initialized = false;
	encoder = NULL;
	ffmpeg_initialized = false;
	networkInitialized = false;
	mappedBuffer = NULL;
}


Kahawai::~Kahawai(void)
{
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
	Map = CreateFileMapping
		(sharedFile,
		NULL,
		PAGE_READWRITE,
		0,
		size,
		_T("SharedFile"));

	if(!Map)
		return false;

	//Define a Mutex for accessing the file map.
	Mutex = CreateMutex(
		NULL,
		FALSE,
		_T("FILE MUTEX"));

	if(!Mutex)
		return false;

	SlaveBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiSlave");

	MasterBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiMaster");


	WaitForSingleObject(Mutex,INFINITE);

	char* b = (char*) MapViewOfFile(Map,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		16);

	if(strcmp(kahawaiMaster,b)!=0)
	{
		strcpy(b,kahawaiMaster);
		Role = Master;
	}
	else
	{
		Role = Slave;
	}

	UnmapViewOfFile(b);
	ReleaseMutex(Mutex);

	return true;
}

/********************************************************************************/
/* Initialization of Kahawai. Must be called before using other Kahawai services*/
/********************************************************************************/
bool Kahawai::Init()
{
	ifstream kahawaiiConfigFile;
	int server;
	string line;


	//TODO:Something much fancier could be done here. 
	{


		kahawaiiConfigFile.open(KAHAWAI_CONFIG,ios::in);
		if(!kahawaiiConfigFile.is_open())
		{
			return false;
		}

		getline(kahawaiiConfigFile,line);
		server = atoi(line.c_str());

		if(!server==1)
		{
			Role = Client;
		}
		else //Otherwise is server, and can be either master or slave process. Determine through mapping
		{
			Role = Master;//for now counts as master. Effective role assigned on InitMapping
		}
			//read the configuration for the lo and hi resolution modes

		if(isClient())
		{
			getline(kahawaiiConfigFile,line);
			strcpy_s(ServerIP,line.c_str());
		}

		getline(kahawaiiConfigFile,line);
		ServerPort=atoi(line.c_str());

		getline(kahawaiiConfigFile,line);
		LoVideoWidth=atoi(line.c_str());

		getline(kahawaiiConfigFile,line);
		LoVideoHeight=atoi(line.c_str());

		getline(kahawaiiConfigFile,line);
		HiVideoWidth=atoi(line.c_str());

		getline(kahawaiiConfigFile,line);
		HiVideoHeight=atoi(line.c_str());

		if(isServer())
		{
			kahawaiiConfigFile.close();
			//Initialize file mapping to share low resolution image between both processes
			return InitMapping((LoVideoHeight * LoVideoWidth * 3)/2);
		}
		else
		{
			kahawaiiConfigFile.close();
			return true;
		}
	}

}


VOID Kahawai::ReadFrameBuffer(int width, int height, byte *buffer) 
{
	glReadBuffer( GL_FRONT );
	glReadPixels( 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer ); 
}

byte Kahawai::delta(byte hi, byte lo)
{
	int resultPixel = ((hi - lo) / 2) + 127;

	if (resultPixel > 255)
		resultPixel = 255;

	if (resultPixel < 0)
		resultPixel = 0;

	return (byte)resultPixel;
}

byte Kahawai::patch(byte delta, byte lo)
{
	int resultPixel = (2 * (delta - 127)) + lo;

	if (resultPixel > 255)
		resultPixel = 255;

	if (resultPixel < 0)
		resultPixel = 0;

	return (byte)resultPixel;
}

void Kahawai::verticalFlip(int width, int height, byte* pixelData, int bitsPerPixel)
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

int Kahawai::initNetwork(){

	if(networkInitialized)
		return true;

	networkInitialized = true;

	//The port you want the server to listen on
	int host_port= ServerPort;

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

		if((kahawaiSocket = accept( hsock, (SOCKADDR*)&sadr, &addr_size))!= INVALID_SOCKET ){
			return 0;
		}
		else
		{
			return -1;
		}
	}
}


bool Kahawai::initializeX264(int width, int height, int fps)
{
	x264_param_t param;


	//define compression parameters
	x264_param_default_preset(&param, "veryfast", "zerolatency");
	param.i_threads = 1;
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
	x264_initialized = true;
	encoder = x264_encoder_open(&param);

	return true;
}

bool Kahawai::initializeFfmpeg()
{
	if(ffmpeg_initialized)
		return 0;

	ffmpeg_initialized = true;

	// Register all formats and codecs
	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	av_register_all();

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}


	pFormatCtx = avformat_alloc_context();
	pFormatCtx->flags |= 0x0080;
	// Open video file
	char url[100];
	sprintf_s(url,100,"tcp://%s:%d",ServerIP,ServerPort);

	if (avformat_open_input(&pFormatCtx, url, NULL, NULL) != 0)
		return false; // Couldn't open file

	opts = setup_find_stream_info_opts(pFormatCtx, codec_opts);
	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, opts) < 0)
		return false; // Couldn't find stream information

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, "kahawai.264", 0);

	// Find the first video stream
	videoStream = -1;
	for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
		if (videoStream == -1)
			return false; // Didn't find a video stream

		// Get a pointer to the codec context for the video stream
		pCodecCtx = pFormatCtx->streams[videoStream]->codec;

		// Find the decoder for the video stream
		pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if (pCodec == NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return false; // Codec not found
		}
		// Open codec
		if (avcodec_open2(pCodecCtx, pCodec, opts) < 0)
			return false; // Could not open codec

		// Allocate video frame
		pFrame = avcodec_alloc_frame();

		// Make a screen to put our video
		screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
		if(!screen) {
			fprintf(stderr, "SDL: could not set video mode - exiting\n");
			exit(1);
		}

		// Allocate a place to put our YUV image on that screen
		bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
			pCodecCtx->height,
			SDL_IYUV_OVERLAY,
			screen);


		return true;
}

bool Kahawai::encodeAndSend(x264_picture_t* pic_in)
{
	int					i, c;
	x264_picture_t		pic_out;
	x264_nal_t*			nals;
	int					i_nals;

	#ifdef WRITE_CAPTURE
	fileSystem->WriteFile( fileName, pic_in.img.plane[0], c );
	#endif

	c = (HiVideoWidth * HiVideoHeight * 3)/2; //YUV420p 4 bytes each 6 pixels

	//Apply delta to each byte of the image
	for (i=0 ; i<c ; i+=3) {
		pic_in->img.plane[0][i] = delta(pic_in->img.plane[0][i],mappedBuffer[i]);
		pic_in->img.plane[0][i+1] = delta(pic_in->img.plane[0][i+1],mappedBuffer[i+1]);
		pic_in->img.plane[0][i+2] = delta(pic_in->img.plane[0][i+2],mappedBuffer[i+2]);
	}


#ifdef WRITE_DELTA
	fileSystem->WriteFile( fileName, pic_in.img.plane[0], c );
#endif

	//Encode the frame
	int frame_size = x264_encoder_encode(encoder, &nals, &i_nals, pic_in, &pic_out);

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
		initNetwork();
		send(kahawaiSocket,(char*) nals[0].p_payload,frame_size,0);
	}

	return true;

}


int Kahawai::decodeAndShow(byte* low,int width, int height)
{
	int frameFinished = 0;

	if(streamFinished)
		return -1;
	// Read frames 
	while(!frameFinished)
	{
		if (av_read_frame(pFormatCtx, &packet) >= 0) 
		{
			// Is this a packet from the video stream?
			if (packet.stream_index == videoStream) 
			{
				// Decode video frame
				avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
				// Did we get a video frame?
				if (frameFinished) 
				{
					SDL_LockYUVOverlay(bmp);

					AVPicture pict;
					pict.data[0] = bmp->pixels[0];
					pict.data[1] = bmp->pixels[1];
					pict.data[2] = bmp->pixels[2];

					pict.linesize[0] = bmp->pitches[0];
					pict.linesize[1] = bmp->pitches[1];
					pict.linesize[2] = bmp->pitches[2];

					// Convert the image into YUV format that SDL uses
					struct SwsContext* img_convert_ctx =
						sws_getContext(pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt,
						pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P,
						SWS_BICUBIC, NULL, NULL, NULL);

					sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,
						0, pCodecCtx->height, pict.data,
						pict.linesize);

					int c = (width * height *3)/2; //YUV420p 4 bytes each 6 pixels
					for (int i=0 ; i<c ; i+=3) {
						pict.data[0][i] = patch(pict.data[0][i],low[i]);
						pict.data[0][i+1] = patch(pict.data[0][i+1],low[i+1]);
						pict.data[0][i+2] = patch(pict.data[0][i+2],low[i+2]);
					}


					SDL_UnlockYUVOverlay(bmp);

					rect.x = 0;
					rect.y = 0;
					rect.w = pCodecCtx->width;
					rect.h = pCodecCtx->height;
					SDL_DisplayYUVOverlay(bmp, &rect);
				}
			}

			// Free the packet that was allocated by av_read_frame
			av_free_packet(&packet);
			// Free the packet that was allocated by av_read_frame
			av_free_packet(&packet);
			SDL_PollEvent(&event);
			switch(event.type) 
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
			av_free(pFrame);

			// Close the codec
			avcodec_close(pCodecCtx);

			// Close the video file
			av_close_input_file(pFormatCtx);
			frameFinished = 1;
			streamFinished = true;
		}

	}

	return 0;
}


void Kahawai::MapRegion()
{
	if(Role==Master)
	{
		//while the slave will write the low detail version to a memory mapped file
		mappedBuffer = (byte*) MapViewOfFile(Map,
			FILE_MAP_READ,
			0,
			0,
			(LoVideoWidth*LoVideoHeight*3)/2);

	}
	if(Role==Slave)
	{
		//while the slave will write the low detail version to a memory mapped file
		mappedBuffer = (byte*) MapViewOfFile(Map,
			FILE_MAP_WRITE,
			0,
			0,
			(LoVideoWidth*LoVideoHeight*3)/2);
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
	if(Role==Master)
	{
		//signal and wait for slave
		SetEvent(MasterBarrier);
		WaitForSingleObject(SlaveBarrier, INFINITE);
	}
	if(Role==Slave)
	{
		//signal and wait for master
		SetEvent(SlaveBarrier);
		WaitForSingleObject(MasterBarrier,INFINITE);
	}

	//3. Get Memory Mapped Region
	if(isServer() && mappedBuffer==NULL)
		MapRegion();

	//4. Transformed (and scale) captured screen to YUV420p at target high resolution
	//Libswscale flips the image when converting it. Flip it in advance, to later get the correct converted image
	verticalFlip(width,height, buffer,3);

	int srcstride = width*3; //RGB stride is just 3*width

	//Allocate the x264 picture structure
	if(!x264_initialized)
		initializeX264(HiVideoWidth,HiVideoHeight);

	x264_picture_alloc(&pic_in, X264_CSP_I420, HiVideoWidth, HiVideoHeight);

	//Convert the image to YUV420
	if(Role==Master)
	{
		convertCtx = sws_getContext(width, height, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	}
	else
	{	//TODO: Need to validate wheter bilinear against bicubic is worth it. (Specially at the client)
		convertCtx = sws_getContext(width, height, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	}

	uint8_t *src[3]= {buffer, NULL, NULL}; 
	sws_scale(convertCtx, src, &srcstride, 0, height, pic_in.img.plane, pic_in.img.i_stride);

	//5. Role specific 
	switch(Role)
	{
	case Master:
		encodeAndSend(&pic_in);
		break;
	case Slave:
		//write to mapped file
		memcpy(mappedBuffer,pic_in.img.plane[0],(width*height*3)/2);
		break;
	case Client:
		initializeFfmpeg();
		decodeAndShow(pic_in.img.plane[0], width, height);
	}

	//6. Cleanup
	if(Role!=Client)
	{
		sws_freeContext(convertCtx);
	}

	x264_picture_clean(&pic_in);
	delete buffer;

}

bool Kahawai::isMaster()
{
	return Role == Master;
}

bool Kahawai::isSlave()
{
	return Role == Slave;
}

bool Kahawai::isClient()
{
	return Role == Client;
}

bool Kahawai::isServer()
{
	return Role != Client;
}

KAHAWAI_MODE Kahawai::getRole()
{
	return Role;
}



AVDictionary* Kahawai::filter_codec_opts(AVDictionary *opts, enum CodecID codec_id,
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
			if (av_opt_find(avcodec_opts[0], t->key, NULL, flags, 0)
				|| (codec && codec->priv_class
				&& av_opt_find(&codec->priv_class, t->key, NULL, flags,
				0)))
				av_dict_set(&ret, t->key, t->value, 0);
			else if (t->key[0] == prefix
				&& av_opt_find(avcodec_opts[0], t->key + 1, NULL, flags, 0))
				av_dict_set(&ret, t->key + 1, t->value, 0);
		}
		return ret;
}

AVDictionary** Kahawai::setup_find_stream_info_opts(AVFormatContext *s,
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
			opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codec->codec_id,
			0);
		return opts;
}

Kahawai kahawai;
