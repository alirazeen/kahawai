#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "FFMpegDecoder.h"
#include "kahawaiBase.h"



FFMpegDecoder::FFMpegDecoder(char* URL, int width, int height)
	:VideoDecoder(),
	_width(width),
	_height(height),
	_streamFinished(false),
	_loaded(false),
	_y420pFrameSize(YUV420pBitsPerPixel(width,height))
{
	_URL = new char[100];
	memcpy(_URL,URL,sizeof(char)*100);

	KahawaiLog("Initializing FFMPEG\n",KahawaiDebug);
	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	av_register_all();

	KahawaiLog("Initializing SDL\n",KahawaiDebug);
	InitializeSDL(width, height);

	_img_convert_ctx =
		sws_getContext(width, height,  PIX_FMT_YUV420P,
		width, height, PIX_FMT_YUV420P,
		SWS_FAST_BILINEAR, NULL, NULL, NULL);

}


FFMpegDecoder::~FFMpegDecoder(void)
{
	delete[] _URL;
}


/**
 * Decodes the incoming stream using FFMPEG. 
 * This method decodes and shows in one step
 * @param	apply	the transform function (if any) to apply to the decoded frame
 * @param	patch	the parameter to the transformation function
 * @return	true if the decoding was successful
 */
bool FFMpegDecoder::Decode(kahawaiTransform apply, byte* patch)
{
	int				frameFinished = 0;
	AVPacket		_Packet;


	if(!_loaded)
	{
		KahawaiLog("Initializing Remote Video Stream\n", KahawaiDebug);
		if(!LoadVideoStream())
			return false;
	}


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
				if(avcodec_decode_video2(_pCodecCtx, _pAVFrame, &frameFinished, &_Packet)<1)
				{
					KahawaiLog("Error decoding video stream", KahawaiError);
					return false;
				}

				// Did we get a video frame?
				if (frameFinished) 
				{
					CopyFrameToOverlay(_pAVFrame,_pYuvOverlay);

					//Patch the frame if is invoked by the delta engine
					if(apply)
					{
						for (int i=0 ; i< _y420pFrameSize ; i++) 
						{
							_pYuvOverlay->pixels[0][i] = apply(_pYuvOverlay->pixels[0][i],patch[i]);
							//_pYuvOverlay->pixels[0][i] = patch[i];
						}
					}

					//writes the raw yuv file to disc if frame logging is enabled
					LogYUVFrame(_saveFrames,"decoded", _displayedFrames, (char*)_pYuvOverlay->pixels[0], _width,_height);

					SDL_DisplayYUVOverlay(_pYuvOverlay, &_screenRect);
					_displayedFrames++;
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

/**
 * NO-OP. This decoder implementation decodes and show in one step
 * @see FFMPEGDecoder::Decode
 * @return always true
 */
bool FFMpegDecoder::Show()
{
	return true;
}

//Transform the AVFrame into a SDL_Overlay
//Uses sws_scale in the process
void FFMpegDecoder::CopyFrameToOverlay(AVFrame* pFrame, SDL_Overlay* pOverlay)
{
	AVPicture pict;

	SDL_LockYUVOverlay(pOverlay);

	pict.data[0] = pOverlay->pixels[0];
	pict.data[1] = pOverlay->pixels[1];
	pict.data[2] = pOverlay->pixels[2];

	pict.linesize[0] = pOverlay->pitches[0];
	pict.linesize[1] = pOverlay->pitches[1];
	pict.linesize[2] = pOverlay->pitches[2];

	//"Conversion" used to place data in the correct struct. Image is already in YUV by this point
	//TODO: This could be optimized
	sws_scale(_img_convert_ctx, pFrame->data, pFrame->linesize,
		0, pFrame->height, pict.data,
		pict.linesize);

	SDL_UnlockYUVOverlay(pOverlay);

}


bool FFMpegDecoder::LoadVideoStream()
{
	AVCodec*			pCodec;
	AVDictionary*		pCodec_opts = NULL;
	AVDictionary**		opts;
	int					opened = -1;

	if(_loaded)
		return true;

	_pFormatCtx = avformat_alloc_context();
	_pFormatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

	for(int attempts = 0; attempts < MAX_ATTEMPTS && opened!=0; attempts++)
	{
		opened = avformat_open_input(&_pFormatCtx, _URL, NULL, NULL);
#ifdef WIN32
		Sleep(1000);
#endif
	}

	if(opened!=0)
	{
		KahawaiLog("Error loading video stream", KahawaiError);
		return false; // Couldn't open file
	}

	opts = SetupFindStreamInfoOptions(_pFormatCtx, pCodec_opts);

	// Raw video. We only have one video stream (FIRST_VIDEO_STREAM)
	// If more streams have to be used. Find the right one iterating through _pFormatCtx->streams
	// Get a pointer to the codec context for the video stream
	_pCodecCtx = _pFormatCtx->streams[FIRST_VIDEO_STREAM]->codec;

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(_pCodecCtx->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return false; // Codec not found
	}
	// Open codec
	if (avcodec_open2(_pCodecCtx, pCodec, opts) < 0)
		return false; // Could not open codec

	// Allocate video frame
	_pAVFrame = avcodec_alloc_frame();

	_loaded = true;

	return true;

}


/************************************************************************/
/* Initializes SDL to show video at the target high resolution          */
/************************************************************************/
bool FFMpegDecoder::InitializeSDL(int width, int height)
{
	//Initialize SDL Library
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		return false;
	}

	// Make a screen to put our video
	_pScreen = SDL_SetVideoMode(width, height, 0, 0);
	if(!_pScreen) {
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		exit(1);
	}

	// Allocate a place to put our YUV image on that screen
	_pYuvOverlay = SDL_CreateYUVOverlay(width,
		height,
		SDL_IYUV_OVERLAY,
		_pScreen);

	//RECT describing the size of the SDL Player
	_screenRect.x = 0;
	_screenRect.y = 0;
	_screenRect.w = width;
	_screenRect.h = height;


	return true;
}



AVDictionary* FFMpegDecoder::FilterCodecOptions(AVDictionary *opts, enum CodecID codec_id,
	int encoder) {
		AVCodecContext*		_avcodec_opts[AVMEDIA_TYPE_NB];
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

AVDictionary** FFMpegDecoder::SetupFindStreamInfoOptions(AVFormatContext *s,
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
#endif