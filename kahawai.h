#ifndef KAHAWAI_H
#define KAHAWAI_H

//Library includes
#include <stdint.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <assert.h>

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

const char kahawaiMaster[8] = "leader\n";

#define KAHAWAI_MAP_FILE "kahawai.dat"
#define KAHAWAI_CONFIG "kahawai.cfg"

enum KAHAWAI_MODE { Master=0, Slave=1, Client=2, Undefined=3};
enum ENCODING_PROFILE { DeltaEncoding, IPFrame};

class Kahawai
{
private:

	//Execution mode

	ENCODING_PROFILE profile;

	//I-Frame encoding state
	int _iFps;
	bool _decodeThreadInitialized;
	byte* _iFrameBuffer;


	//Kahawai Rendered Frames
	int _renderedFrames;
	int _fps; //target game FPS
	bool _offloading;
	int _decodedServerFrames;

	//time Interception
	int _sys_timeBase;
	int _sys_prevFrame;
	int _sys_prevTime;
	bool _timeInitialized;
	int _timeStep;

	//SERVER IPC
	KAHAWAI_MODE _role;
	HANDLE _mutex;
	HANDLE _map;
	HANDLE _slaveBarrier;
	HANDLE _masterBarrier;
	byte* _mappedBuffer;

	//VIDEO QUALITY SETTINGS
	int _loVideoHeight;
	int _loVideoWidth;
	int _hiVideoHeight;
	int _hiVideoWidth;
	int _iFrameSkip;
	int _hiFrameSize;

	//NETWORK SETTINGS
	int _serverPort;
	char _serverIP[75];

	//VIDEO ENCODING/DECODING STATE
	bool _x264_initialized;
	x264_t* _encoder;
	bool _ffmpeg_initialized;
	bool _networkInitialized;
	bool _streamFinished;


	//VIDEO STREAMING SERVER SOCKET
	SOCKET _socket;

	//FFMPEG State
	AVCodecContext*		_avcodec_opts[AVMEDIA_TYPE_NB];
	AVDictionary*		_pFormat_opts;
	AVDictionary*		_pCodec_opts;
	AVFormatContext*	_pFormatCtx;
	int					_pVideoStream;
	AVCodecContext*		_pCodecCtx;
	AVFrame*			_pFrame;
	AVCodec*			_pCodec;
	AVDictionary**		opts;

	//SDL Video player settings
	SDL_Overlay*		_pBmp;
	SDL_Surface*		_pScreen;
	SDL_Rect			_screenRect;

	AVDictionary *FilterCodecOptions(AVDictionary *opts, enum CodecID codec_id,
		int encoder);

	AVDictionary **SetupFindStreamInfoOptions(AVFormatContext *s,
		AVDictionary *pCodec_opts);

	bool InitMapping(int size);
	void MapRegion();
	void ReadFrameBuffer( int width, int height, byte *buffer);
	int InitNetwork();
	bool InitializeX264(int width, int height, int fps=60);
	bool InitializeFfmpeg();
	bool InitializeIFrameSharing();
	int DecodeAndShow(byte* low,int width, int height);
	int DecodeAndMix(int width, int height);
	bool EncodeAndSend(x264_picture_t* pic_in);
	bool EncodeIFrames(x264_picture_t* pic_in);


public:
	Kahawai(void);
	~Kahawai(void);
	bool Init();
	bool IsMaster();
	bool IsSlave();
	bool IsClient();
	bool IsServer();
	bool IsDelta();
	bool IsIFrame();
	bool IsOffloading();
	ENCODING_PROFILE GetMode();

	KAHAWAI_MODE GetRole();

	void OffloadVideo( int width, int height, int frameNumber);
	void CaptureDelta( int width, int height, int frameNumber);
	void CaptureIFrame( int width, int height, int frameNumber);
	int Sys_Milliseconds();

};

extern Kahawai kahawai;


#endif