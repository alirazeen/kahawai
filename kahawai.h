#ifndef KAHAWAI_H
#define KAHAWAI_H

//Library includes
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

//Defines, enums and constants
#define MAPPING_SIZE (1024*768*3)+18
const char kahawaiMaster[8] = "leader\n";

#define KAHAWAI_MAP_FILE "kahawai.dat"
#define KAHAWAI_CONFIG "kahawai.cfg"

enum KAHAWAI_MODE { Master=0, Slave=1, Client=2, Undefined=3};
enum ENCODING_PROFILE { Default, IPFrame};


class Kahawai
{
private:



	//SERVER IPC
	KAHAWAI_MODE Role;
	HANDLE Mutex;
	HANDLE Map;
	HANDLE SlaveBarrier;
	HANDLE MasterBarrier;
	byte* mappedBuffer;

	//VIDEO QUALITY SETTINGS
	int LoVideoHeight;
	int LoVideoWidth;
	int HiVideoHeight;
	int HiVideoWidth;
	int IFrameSkip;

	//NETWORK SETTINGS
	int ServerPort;
	char ServerIP[75];

	//VIDEO ENCODING/DECODING STATE
	bool x264_initialized;
	x264_t* encoder;
	bool ffmpeg_initialized;
	bool networkInitialized;
	bool streamFinished;


	//VIDEO STREAMING SERVER SOCKET
	SOCKET kahawaiSocket;

	//FFMPEG State
	AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
	AVDictionary *format_opts, *codec_opts;
	AVFormatContext *pFormatCtx;
	int videoStream;
	AVPacket packet;
	AVCodecContext *pCodecCtx;
	AVFrame *pFrame;
	AVCodec *pCodec;
	AVDictionary **opts;

	//SDL Video player settings
	SDL_Overlay     *bmp;
	SDL_Surface     *screen;
	SDL_Rect        rect;
	SDL_Event       event;

	AVDictionary *filter_codec_opts(AVDictionary *opts, enum CodecID codec_id,
		int encoder);

	AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
		AVDictionary *codec_opts);

	bool InitMapping(int size);
	void MapRegion();
	void ReadFrameBuffer( int width, int height, byte *buffer);
	byte delta(byte hi, byte lo);
	byte patch(byte delta, byte lo);
	void verticalFlip(int width, int height, byte* pixelData, int bitsPerPixel);
	int initNetwork();
	bool initializeX264(int width, int height, int fps=60);
	bool initializeFfmpeg();
	int decodeAndShow(byte* low,int width, int height);
	bool encodeAndSend(x264_picture_t* pic_in);


public:
	Kahawai(void);
	~Kahawai(void);
	bool Init();
	bool isMaster();
	bool isSlave();
	bool isClient();
	bool isServer();
	KAHAWAI_MODE getRole();

	void CaptureDelta( int width, int height, int frameNumber);

};

extern Kahawai kahawai;


#endif