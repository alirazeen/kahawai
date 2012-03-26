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
	kahawaiMap = CreateFileMapping
		(sharedFile,
		NULL,
		PAGE_READWRITE,
		0,
		size,
		_T("SharedFile"));

	if(!kahawaiMap)
		return false;

	//Define a Mutex for accessing the file map.
	kahawaiMutex = CreateMutex(
		NULL,
		FALSE,
		_T("FILE MUTEX"));

	if(!kahawaiMutex)
		return false;

	kahawaiSlaveBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiSlave");

	kahawaiMasterBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiMaster");


	WaitForSingleObject(kahawaiMutex,INFINITE);

	char* b = (char*) MapViewOfFile(kahawaiMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		16);

	if(strcmp(kahawaiMaster,b)!=0)
	{
		strcpy(b,kahawaiMaster);
		kahawaiProcessId = Master;
	}
	else
	{
		kahawaiProcessId = Slave;
	}

	UnmapViewOfFile(b);
	ReleaseMutex(kahawaiMutex);

	return true;
}

bool Kahawai::Init()
{
	ifstream kahawaiiConfigFile;
	int server;
	string line;

	char loVideoMode;
	char hiVideoMode;

	//TODO:Something much fancier could be done here. but this will	if(fileSystem->IsInitialized())
	{


		kahawaiiConfigFile.open("kahawai.cfg",ios::in);
		if(!kahawaiiConfigFile.is_open())
		{
			return false;
		}

		getline(kahawaiiConfigFile,line);
		server = atoi(line.c_str());

		if(!server==1)
		{
			kahawaiProcessId = Client;
			getline(kahawaiiConfigFile,line);
			strcpy_s(kahawaiServerIP,line.c_str());

			getline(kahawaiiConfigFile,line);
			kahawaiServerPort=atoi(line.c_str());

			kahawaiiConfigFile.close();
			return true;
		}
		else //Otherwise is server, and can be either master or slave process. Determine through mapping
		{
			//read the configuration for the lo and hi resolution modes

			getline(kahawaiiConfigFile,line);
			kahawaiServerPort=atoi(line.c_str());

			getline(kahawaiiConfigFile,line);
			kahawaiLoVideoWidth=atoi(line.c_str());

			getline(kahawaiiConfigFile,line);
			kahawaiLoVideoHeight=atoi(line.c_str());

			getline(kahawaiiConfigFile,line);
			kahawaiHiVideoWidth=atoi(line.c_str());

			getline(kahawaiiConfigFile,line);
			kahawaiHiVideoHeight=atoi(line.c_str());

			kahawaiiConfigFile.close();
			//Initialize file mapping to share low resolution image between both processes
			return InitMapping((kahawaiLoVideoHeight * kahawaiLoVideoWidth * 3)/2);
		}
		return 0;
	}

}


VOID Kahawai::ReadFrameBuffer(int width, int height, byte *buffer) {
	//session->UpdateScreen();
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
	int host_port= kahawaiServerPort;

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
	x264_nal_t *headers;
	int i_nal;

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
	//x264_encoder_headers( encoder, &headers, &i_nal );

	//idFile* movieOut;
	//movieOut = fileSystem->WriteFile("kahawai.h264",headers->p_payload)


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
	sprintf_s(url,100,"tcp://%s:%d",kahawaiServerIP,kahawaiServerPort);

	if (avformat_open_input(&pFormatCtx, url, NULL, NULL) != 0)
		return -1; // Couldn't open file

	opts = setup_find_stream_info_opts(pFormatCtx, codec_opts);
	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, opts) < 0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, "kahawai.264", 0);

	// Find the first video stream
	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
		if (videoStream == -1)
			return -1; // Didn't find a video stream

		// Get a pointer to the codec context for the video stream
		pCodecCtx = pFormatCtx->streams[videoStream]->codec;

		// Find the decoder for the video stream
		pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if (pCodec == NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return -1; // Codec not found
		}
		// Open codec
		if (avcodec_open2(pCodecCtx, pCodec, opts) < 0)
			return -1; // Could not open codec

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
				i++;
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
					for (i=0 ; i<c ; i+=3) {
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
			// Free the RGB image

			// Free the YUV frame
			av_free(pFrame);

			// Close the codec
			avcodec_close(pCodecCtx);

			// Close the video file
			av_close_input_file(pFormatCtx);
			frameFinished = 1;
			streamFinished;
		}

	}

	return 0;
}

/*
================== 
CaptureDelta

Move to tr_imagefiles.c...

Will automatically tile render large screen shots if necessary
Downsample is the number of steps to mipmap the image before saving it
If ref == NULL, session->updateScreen will be used
================== 
*/  
void Kahawai::TakeScreenshot( int width, int height, const char *fileName, int blends) {
	byte		*buffer;
	byte		*loBuffer; //only used by master
	int			i, j, c;
	x264_picture_t pic_in, pic_out;

	//Synchronize both processes if running at the server
	if(kahawaiProcessId==Master)
	{
		if(!x264_initialized)
			initializeX264(width,height);

		//signal and wait for slave
		SetEvent(kahawaiMasterBarrier);
		WaitForSingleObject(kahawaiSlaveBarrier, INFINITE);
	}
	if(kahawaiProcessId==Slave)
	{
		//signal and wait for master
		SetEvent(kahawaiSlaveBarrier);
		WaitForSingleObject(kahawaiMasterBarrier,INFINITE);
	}

	int	pix = width * height;

	buffer =  new byte[3*pix];

	ReadFrameBuffer( width, height, buffer);

	//If master Read Low definition
	if(kahawaiProcessId==Master)
	{
		//while the slave will write the low detail version to a memory mapped file
		loBuffer = (byte*) MapViewOfFile(kahawaiMap,
			FILE_MAP_READ,
			0,
			0,
			(pix*3)/2);

	}
	if(kahawaiProcessId==Slave)
	{
		//while the slave will write the low detail version to a memory mapped file
		loBuffer = (byte*) MapViewOfFile(kahawaiMap,
			FILE_MAP_WRITE,
			0,
			0,
			(pix*3)/2);
	}


	verticalFlip(width,height, buffer,3);

	// _D3XP adds viewnote screenie save to cdpath
	uint8_t;
	//data is a pointer to you RGB structure
	x264_picture_alloc(&pic_in, X264_CSP_I420, width, height);
	int srcstride = width*3; //RGB stride is just 3*width

	//TODO: (KAHAWAI) Change the context in slave mode to adjust the input resolution
	struct SwsContext* convertCtx = sws_getContext(width, height, PIX_FMT_RGB24, width, height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

	uint8_t *src[3]= {buffer, NULL, NULL}; 
	sws_scale(convertCtx, src, &srcstride, 0, height, pic_in.img.plane, pic_in.img.i_stride);

	if(kahawaiProcessId==Slave)
	{
		//write to mapped file
		memcpy(loBuffer,pic_in.img.plane[0],(width*height*3)/2);
	}

	//If client read from socket
	if(kahawaiProcessId==Client)
	{

		initializeFfmpeg();
		decodeAndShow(pic_in.img.plane[0], width, height);


	}


	if(kahawaiProcessId==Master)
	{

		c = (width * height * 3)/2; //YUV420p 4 bytes each 6 pixels
		//#ifdef WRITE_CAPTURE
		//fileSystem->WriteFile( fileName, pic_in.img.plane[0], c );
		//#endif

		for (i=0 ; i<c ; i+=3) {
			pic_in.img.plane[0][i] = delta(pic_in.img.plane[0][i],loBuffer[i]);
			pic_in.img.plane[0][i+1] = delta(pic_in.img.plane[0][i+1],loBuffer[i+1]);
			pic_in.img.plane[0][i+2] = delta(pic_in.img.plane[0][i+2],loBuffer[i+2]);
		}



#ifdef WRITE_DELTA
		fileSystem->WriteFile( fileName, pic_in.img.plane[0], c );
#endif
		x264_nal_t* nals;
		int i_nals;

		int frame_size = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);

		//this should go to a socket or to rtsp
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



	}


	if(kahawaiProcessId!=Client)
	{
		//Free x264 pictures
		sws_freeContext(convertCtx);
		UnmapViewOfFile(loBuffer);
	}

	x264_picture_clean(&pic_in);
	delete buffer;

}

bool Kahawai::isMaster()
{
	return kahawaiProcessId == Master;
}

bool Kahawai::isSlave()
{
	return kahawaiProcessId == Slave;
}

bool Kahawai::isClient()
{
	return kahawaiProcessId == Client;
}

KAHAWAI_MODE Kahawai::getRole()
{
	return kahawaiProcessId;
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
		int i;
		AVDictionary **opts;

		if (!s->nb_streams)
			return NULL;
		opts = (AVDictionary**)av_mallocz(s->nb_streams * sizeof(*opts));
		if (!opts) {
			av_log(NULL, AV_LOG_ERROR,
				"Could not alloc memory for stream options.\n");
			return NULL;
		}
		for (i = 0; i < s->nb_streams; i++)
			opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codec->codec_id,
			0);
		return opts;
}

Kahawai kahawai;
