#pragma once
#include "videodecoder.h"

#define MAX_ATTEMPTS 10

//SDL Imports
#include <SDL.h>
#include <SDL_thread.h>

//FFMPEG Imports
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

//FFMPEG Constants
#define FIRST_VIDEO_STREAM 0

class FFMpegDecoder :
	public VideoDecoder
{
public:
	FFMpegDecoder(char* URL, int width, int height);
	~FFMpegDecoder(void);
	bool Decode(transform apply = 0, byte* patch = 0); 
	bool Show(); 
	AVDictionary* FilterCodecOptions(AVDictionary *opts, enum CodecID codec_id,	int encoder);
	AVDictionary** SetupFindStreamInfoOptions(AVFormatContext *s, AVDictionary *codec_opts);
protected:
	bool InitializeSDL(int width, int height);
	bool LoadVideoStream();
	void CopyFrameToOverlay(AVFrame* pFrame, SDL_Overlay* pOverlay);

	//Media source
	char* _URL;
	int _y420pFrameSize; //The size of the frame in YUV420p format
	bool _streamFinished; //Signals whether the end of the stream has been read
	bool _loaded;		  //has the media URL been loaded?

	//SDL Video player settings
	SDL_Overlay*		_pYuvOverlay;
	SDL_Surface*		_pScreen;
	SDL_Rect			_screenRect;

	//FFMPEG State
	AVFormatContext*	_pFormatCtx;
	AVCodecContext*		_pCodecCtx;
	AVFrame*			_pAVFrame;
	AVDictionary**		opts;

	//Screen resolution
	int					_width;
	int					_height;

	//SWS Context
	SwsContext* _img_convert_ctx;

};

