#ifndef KAHAWAI_H
#define KAHAWAI_H

#include <stdint.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_thread.h>

extern "C" {
#include "x264.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <windows.h>
#include <tchar.h>

#if defined( _WIN32 )

#include <gl/gl.h>

#elif defined( MACOS_X )

// magic flag to keep tiger gl.h from loading glext.h
#define GL_GLEXT_LEGACY
#include <OpenGL/gl.h>

#elif defined( __linux__ )

// using our local glext.h
// http://oss.sgi.com/projects/ogl-sample/ABI/
#define GL_GLEXT_LEGACY
#define GLX_GLXEXT_LEGACY
#include <GL/gl.h>
#include <GL/glx.h>

#else

#include <gl.h>

#endif


enum KAHAWAI_MODE { Master=0, Slave=1, Client=2, Undefined=3};
#define MAPPING_SIZE (1024*768*3)+18

#define KAHAWAI_MAP_FILE "kahawai.dat"

const char kahawaiMaster[8] = "leader\n";



class Kahawai
{
private:



	//SERVER IPC
	KAHAWAI_MODE kahawaiProcessId;
	HANDLE kahawaiMutex;
	HANDLE kahawaiMap;
	HANDLE kahawaiSlaveBarrier;
	HANDLE kahawaiMasterBarrier;

	//VIDEO QUALITY SETTINGS
	int kahawaiLoVideoHeight;
	int kahawaiLoVideoWidth;
	int kahawaiHiVideoHeight;
	int kahawaiHiVideoWidth;
	int kahawaiIFrameSkip;

	//NETWORK SETTINGS
	int kahawaiServerPort;
	char kahawaiServerIP[75];

	//VIDEO ENCODING/DECODING STATE
	bool x264_initialized;
	x264_t* encoder;
	bool ffmpeg_initialized;
	bool networkInitialized;
	bool streamFinished;

	//VIDEO STREAMING SERVER SOCKET
	SOCKET kahawaiSocket;


	AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
	AVDictionary *format_opts, *codec_opts;
	AVFormatContext *pFormatCtx;
	int i, videoStream;
	AVPacket packet;
	AVCodecContext *pCodecCtx;
	AVFrame *pFrame;

	AVCodec *pCodec;
	AVDictionary **opts;

	SDL_Overlay     *bmp;
	SDL_Surface     *screen;
	SDL_Rect        rect;
	SDL_Event       event;

	AVDictionary *filter_codec_opts(AVDictionary *opts, enum CodecID codec_id,
		int encoder);

	AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
		AVDictionary *codec_opts);

	bool InitMapping(int size);
	void ReadFrameBuffer( int width, int height, byte *buffer);
	byte delta(byte hi, byte lo);
	byte patch(byte delta, byte lo);
	void verticalFlip(int width, int height, byte* pixelData, int bitsPerPixel);
	int initNetwork();
	bool initializeX264(int width, int height, int fps=60);
	bool initializeFfmpeg();
	int decodeAndShow(byte* low,int width, int height);



public:
	Kahawai(void);
	~Kahawai(void);
	bool Init();
	bool isMaster();
	bool isSlave();
	bool isClient();
	KAHAWAI_MODE getRole();

	void TakeScreenshot( int width, int height, const char *fileName, int blends);

};

extern Kahawai kahawai;


#endif