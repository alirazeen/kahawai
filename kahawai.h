#ifndef KAHAWAI_H
#define KAHAWAI_H

#ifdef _DEBUG
#ifndef VERIFY
#define VERIFY(f)          assert(f)
#endif
#else
#ifndef VERIFY
#define VERIFY(f)          ((void)(f))
#endif
#endif 

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
#define FIRST_VIDEO_STREAM 0
#define IFRAME_MUTEX "IFRAME"


enum KAHAWAI_MODE { Master=0, Slave=1, Client=2, Undefined=3};
enum ENCODING_PROFILE { DeltaEncoding, IPFrame};

class Kahawai
{
private:

	//Execution mode

	ENCODING_PROFILE profile;

	//I-Frame encoding state
	int _iFps;
	bool _crossStreamInitialized;
	byte* _iFrameBuffer;
	byte* _pFrameBuffer;
	int _iFrameBufferSize;
	bool _loadedPStream;
	bool _serverSocketInitialized;
	SRWLOCK _renderedFramesLock;
	SRWLOCK _iFrameBufferLock;
	CONDITION_VARIABLE _iFrameBufferCV;
	int _lastIFrameMerged;

	//Delta encoding state
	bool _loadedDeltaStream;

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
	bool _networkInitialized;

	//VIDEO ENCODING/DECODING STATE
	bool _x264_initialized;
	x264_t* _encoder;
	bool _ffmpeg_initialized;
	bool _socketToClientInitialized;
	bool _sdlInitialized;
	bool _streamFinished;
	bool _decodeThreadInitialized;


	//VIDEO STREAMING SERVER SOCKET
	SOCKET _clientSocket;
	SOCKET _serverSocket;

	//FFMPEG State
	AVFormatContext*	_pFormatCtx;
	AVCodecContext*		_pCodecCtx;
	AVFrame*			_pFrame;
	AVDictionary**		opts;

	//SDL Video player settings
	SDL_Overlay*		_pYuvOverlay;
	SDL_Surface*		_pScreen;
	SDL_Rect			_screenRect;

	AVDictionary *FilterCodecOptions(AVDictionary *opts, enum CodecID codec_id,
		int encoder);

	AVDictionary **SetupFindStreamInfoOptions(AVFormatContext *s,
		AVDictionary *pCodec_opts);

	bool InitClientIFrame();
	bool InitMapping(int size);
	void MapRegion();
	void ReadFrameBuffer( int width, int height, byte *buffer);
	bool InitSocketToClient();
	bool InitServerSocket();

	bool InitializeX264(int width, int height, int fps=60);
	bool InitializeFfmpeg();
	bool InitializeIFrameSharing();
	bool InitializeSDL();
	bool InitNetworking();

	bool DecodeAndShow(int width, int height,byte* low);
	bool DecodeAndMix(int width, int height);
	bool EncodeAndSend(x264_picture_t* pic_in);
	bool EncodeIFrames(x264_picture_t* pic_in);
	bool EncodeServerFrames(x264_picture_t* pic_in);

	//I-P Frame IPC
	static DWORD WINAPI Kahawai::CrossStreamsThreadStart(void* Param);
	static DWORD WINAPI Kahawai::DecodeThreadStart(void* Param);



	DWORD CrossStreams(void);
	bool InitDecodeThread();

	bool LoadVideo(char* IP, int port, 
		AVFormatContext**	pFormatCtx, 
		AVCodecContext**	pCodecCtx, 
		AVFrame**			pFrame);

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

	void OffloadVideo( int width, int height);
	void CaptureDelta( int width, int height);
	void CaptureIFrame( int width, int height);
	int Sys_Milliseconds();
	bool skipFrame();

};

extern Kahawai kahawai;


#endif