#include"kahawaiBase.h"
#ifdef KAHAWAI
#include "Kahawai.h"
#include "DeltaClient.h"
#include "DeltaServer.h"
#include "IFrameClient.h"
#include "IFrameServer.h"
#include "H264Client.h"
#include "H264Server.h"
#include "ConfigReader.h"
#include "ConfigReader.h"
#include "OpenGLCapturer.h"

#include<iostream>
#include<fstream>
#include<string>

using namespace std;

//////////////////////////////////////////////////////////////////////////
// Kahawai Public Interface
//////////////////////////////////////////////////////////////////////////

/**
 * Starts the synchronous portion of the offload. Should be used as a hook 
 * This method must be called after Logic / Render are completed
 * Continues with Capture and ensures that an async thread is started 
 * for the other stages
 * @param the framebuffer captured from the game
 * @return true if the synchronous offload succeeded
 */
bool Kahawai::Offload()
{
	if(!_offloading)
	{
		if(!StartOffload())
			return false;
	}

	//Start measuring time after 100 frames have been rendered
	//to ingore initial warm up time
	if(_renderedFrames == 100)
	{
		_offloadStartTime = timeGetTime();
	}

#if _DEBUG
	
	if(_renderedFrames == 6000)
	{
		LogFPS();
		return false;
	}
#endif


	//////////////////////////////////////////////////////////////////////////
	// Kahawai LifeCycle starts here.
	// Logic/Render/Capture are done in this thread
	// Transform/Encode/Send for the server
	// Transform/Decode/Show for the client
	// are done in the other thread (Implemented in OffloadAsync)
    //////////////////////////////////////////////////////////////////////////
	Capture(_width, _height);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Initialization / Configuration
//////////////////////////////////////////////////////////////////////////

/**
 * Creates a new Kahawai instance using the default config file (Kahawai.xml)
 */
Kahawai* Kahawai::LoadFromFile()
{
	Kahawai* instance = NULL;
	char valueBuffer[15];
	ENCODING_MODE encodingMode;
	boolean server;

	KahawaiLog("Loading Kahawai instance", KahawaiDebug);

	ConfigReader* reader = new ConfigReader(KAHAWAI_CONFIG);
	
	//Read role
	reader->ReadProperty(CONFIG_OFFLOAD,CONFIG_ROLE,valueBuffer);
	server = _strnicmp(valueBuffer,CONFIG_SERVER,sizeof(CONFIG_SERVER))==0;

	//Read curent instance encoding technique
	reader->ReadProperty(CONFIG_OFFLOAD,CONFIG_TECHNIQUE,valueBuffer);
	if(_strnicmp(valueBuffer,CONFIG_DELTA,sizeof(CONFIG_DELTA))==0)
		encodingMode = DeltaEncoding;

	if(_strnicmp(valueBuffer,CONFIG_IFRAME,sizeof(CONFIG_IFRAME))==0)
		encodingMode = IPFrame;

	if(_strnicmp(valueBuffer,CONFIG_H264,sizeof(CONFIG_H264))==0)
		encodingMode = H264;



	//Create an appropriate instance
	if(encodingMode == DeltaEncoding)
	{
		if(server)
			instance = new DeltaServer();
		else
			instance = new DeltaClient();
	}

	if(encodingMode == IPFrame)
	{
		if(server)
			instance = new IFrameServer();
		else
			instance = new IFrameClient();
	}

	if(encodingMode == H264)
	{
		if(server)
			instance = new H264Server();
		else
			instance = new H264Client();
	}

	instance->SetReader(reader);
	if(!instance->Initialize())
		return NULL;

	delete reader;

	return instance;

}


/**
 * Starts the Offloading Pipeline Asynchronously
 */
bool Kahawai::StartOffload()
{
	_renderedFrames = 0;

	bool result = CreateKahawaiThread(StartPipeline,this);
	_offloading=true;
	if(!result)
	{
		KahawaiLog("Fatal Error: Unable to create offload async thread\n", KahawaiError);
		return false;
	}

	return true;
}

/**
 * Calls the appropriate overload to handle offloading asynchronously
 */
DWORD WINAPI Kahawai::StartPipeline(void* Param) 
{
	Kahawai* This = (Kahawai*) Param;	
	This->OffloadAsync();

	return 0;
}

bool Kahawai::IsOffloading()
{
	return _offloading;
}

/**
 * By default, frames should not be skipped. Override this method to provide
 * custom behavior (I
 *
 */
bool Kahawai::ShouldSkip()
{
	return false;
}

bool Kahawai::StopOffload()
{
	_offloading = false;
	KahawaiLog("Offload stopped gracefully", KahawaiDebug);
	return true;
}


//////////////////////////////////////////////////////////////////////////
// Time Control
//////////////////////////////////////////////////////////////////////////

int Kahawai::Sys_Milliseconds( void ) {
	int sys_curtime;
	static int sys_timeBase;
	static int sys_prevFrame;
	static int sys_prevTime;
	static bool	timeInitialized = false;
	if ( !timeInitialized ) {
		sys_timeBase = timeGetTime();
		sys_prevFrame = _renderedFrames; //CUERVO:THis may be a synchronization issue
		sys_prevTime = sys_timeBase;
		timeInitialized = true;
	}
	int frame = _renderedFrames;

	if(frame > sys_prevFrame)
	{
		sys_curtime = sys_prevTime + ((frame - sys_prevFrame) * _timeStep);
		sys_prevFrame = frame;
		sys_prevTime = sys_curtime;
	}
	else
	{
		sys_curtime = sys_prevTime;
	}

	return sys_curtime; 
}


//////////////////////////////////////////////////////////////////////////

/************************************************************************/
/* Kahawai Base Constructors                                            */
/************************************************************************/
Kahawai::Kahawai()
	:_offloading(false)
	,_mode(DeltaEncoding)
	,_height(0)
	,_width(0)
	,_fps(60)
	,_bitDepth(3)
	,_timeStep(1000/_fps)
	,_renderedFrames(0)
	,_serverPort(KAHAWAI_DEFAULT_PORT)
	,_captureMode(OpenGL)
	,_frameInProcess(false)
	,_capturer(NULL)
	,_finished(false)
{

}

Kahawai::~Kahawai(void)
{
	if(!_finished)
	{
		Finalize();
	}

	if(_sourceFrame!=NULL)
		delete[] _sourceFrame;

	if(_capturer!=NULL)
		delete _capturer;

	_sourceFrame = NULL;
	_capturer = NULL;

}

//////////////////////////////////////////////////////////////////////////
// Main Kahawai Pipeline (Common)
//
//  Capture -> Transform -> Decode -> Show  (Client)
//  Capture -> Transform -> Encode -> Send  (Server)
//////////////////////////////////////////////////////////////////////////


/**
 * Base initializer for all Kahawai Objects
 * Reads the general attributes from the config file
 */
bool Kahawai::Initialize()
{

	//Read Screen Resolution
	_width = _configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_WIDTH);
	_height = _configReader->ReadIntegerValue(CONFIG_RESOLUTION,CONFIG_HEIGHT);

	//Read Server Port (Client also needs to know IP. Server uses its own IP)
	_serverPort = _configReader->ReadIntegerValue(CONFIG_SERVER,CONFIG_SERVER_PORT);

	//Read Game Settings
	_configReader->ReadProperty(CONFIG_OFFLOAD,CONFIG_GAME,_gameName);

	if(_strnicmp(_gameName, CONFIG_DOOM3,sizeof(CONFIG_DOOM3))==0)
	{
		_configReader->ReadProperty(CONFIG_DOOM3,CONFIG_DEMO_FILE,_demoFile);
	}
	else if(_strnicmp(_gameName, CONFIG_DOOM3,sizeof(CONFIG_SF4))==0)
	{
		KahawaiLog("Street Fighter IV hasn't been fully implemented yet. Aborting", KahawaiError);
		return false;
	}
	else
	{
		KahawaiLog("Only Doom 3 and Street Fighter 4 are currently supported by Kahawai. Aborting", KahawaiError);
		return false;
	}

	//Load the specified capturer

	if(_captureMode == OpenGL)
	{
		_capturer = new OpenGLCapturer(_width,_height);
	}
	else
	{
		KahawaiLog("Only the OpenGL capturer is currently implemented", KahawaiError);
		return false;
	}

#ifdef _DEBUG
	//Read Debug settings
	_configReader->ReadProperty(CONFIG_DEBUG,CONFIG_RESULTS_PATH,g_resultsPath);
	_saveCaptures = _configReader->ReadBooleanValue(CONFIG_DEBUG,CONFIG_SAVE_FRAME);


	//Read encoder settings (only to determine the save path for the dumped frames)
	int preset, crf;
	preset = _configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_ENCODER_LEVEL);
	crf = _configReader->ReadIntegerValue(CONFIG_ENCODER,CONFIG_CRF);
	char technique[15];
	_configReader->ReadProperty(CONFIG_OFFLOAD,CONFIG_TECHNIQUE,technique);

	sprintf_s(g_resultsPath,"%s\\%s\\%s\\Low\\%s\\%d",g_resultsPath,_demoFile, technique,x264_preset_names[preset],crf);
#endif

	//Structure used in all derived classes mostly for libsws colorspace
	_transformPicture = new x264_picture_t;
	x264_picture_alloc(_transformPicture, X264_CSP_I420, _width, _height);

	//Initialize sws scaling context
	_convertCtx = sws_getContext(_width,_height,PIX_FMT_BGRA, _width, _height,PIX_FMT_YUV420P, SWS_FAST_BILINEAR,NULL,NULL,NULL);

	//Initialize Networking
	InitNetworking();

	//Initialize buffer for captured frames
	_sourceFrame = new uint8_t[_width*_height*SOURCE_BITS_PER_PIXEL];


	//Initialize Synchronization
	InitializeCriticalSection(&_frameBufferCS);
	InitializeConditionVariable(&_captureFullCV);
	InitializeConditionVariable(&_captureReadyCV);

	return true;	
}

bool Kahawai::Finalize()
{
	WakeConditionVariable(&_captureReadyCV);
	WakeConditionVariable(&_captureFullCV);
	_finished = true;

	return true;
}


/**
 * Copies the contents of the framebuffer into system memory
 * It is responsible for controlling concurrency between Capture and Transform stages
 * @param width the width of the target screen to be captured
 * @param height the height of the target screen to be captured
 * @return true if the capture process was successful
 */
bool Kahawai::Capture(int width, int height)
{
	uint8_t* frameBuffer = _capturer->CaptureScreen();

	//Acquire lock on the framebuffer. Make sure transform thread is done

	//This is how locking primitives work in Windows. Although the order may seem counterintuitive
	//http://msdn.microsoft.com/en-us/library/windows/desktop/ms686353(v=vs.85).aspx

	EnterCriticalSection(&_frameBufferCS);
	{
		while(_frameInProcess)
		{
			SleepConditionVariableCS(&_captureFullCV,&_frameBufferCS,INFINITE);
			if(!_offloading)
				return false;
		}
		memcpy(_sourceFrame,frameBuffer, width* height*SOURCE_BITS_PER_PIXEL);
		_frameInProcess = true;
	}
	WakeConditionVariable(&_captureReadyCV);
	LeaveCriticalSection(&_frameBufferCS);

	//Increase the count of rendered frames sent to the Kahawai engine.
	_renderedFrames++;

	return true;
}


/**
 * Color converts the captured buffer from RGB to YUV420p 
 * @param width the target screen width
 * @param height the target screen height
 * @return true if the conversion is successful
 */
bool Kahawai::Transform(int width, int height)
{
	//Acquire lock on the framebuffer. Wait until game has produced a new frame

	EnterCriticalSection(&_frameBufferCS);
	{
		while(!_frameInProcess)
		{
			SleepConditionVariableCS(&_captureReadyCV,&_frameBufferCS,INFINITE);
			if(!_offloading)
				return false;
		}

		//Image colorspace transformation inverts image. Pre-inverting to save time
		VerticalFlip(width, height, _sourceFrame, SOURCE_BITS_PER_PIXEL);
		int srcstride = _width * SOURCE_BITS_PER_PIXEL; //RGB Stride
		uint8_t *src[3]= {_sourceFrame, NULL, NULL}; 
		sws_scale(_convertCtx, src, &srcstride, 0, height, _transformPicture->img.plane, _transformPicture->img.i_stride);

		LogYUVFrame(_saveCaptures,"low",_renderedFrames,(char*)_transformPicture->img.plane[0],_width,_height);

		_frameInProcess=false;
	}
	//Wake the game thread if it is waiting to send more frames.
	WakeConditionVariable(&_captureFullCV);
	LeaveCriticalSection(&_frameBufferCS);

	return true;

}

//////////////////////////////////////////////////////////////////////////
// Accessors / Mutators
//////////////////////////////////////////////////////////////////////////

/**
 * Sets the reader object for this Kahawai object
 * Must be set before using the object
 */
void Kahawai::SetReader(ConfigReader* reader)
{
	_configReader = reader;
}

void Kahawai::SetCaptureMode(CAPTURE_MODE mode)
{
	_captureMode = mode;
}

int Kahawai::GetWidth()
{
	return _width;
}

int Kahawai::GetHeight()
{
	return _height;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Input
//////////////////////////////////////////////////////////////////////////

bool Kahawai::ShouldHandleInput()
{
	return GetDisplayedFrames()>=GetFirstInputFrame();
}

//////////////////////////////////////////////////////////////////////////
// Instrumentation
//////////////////////////////////////////////////////////////////////////

void Kahawai::LogFPS()
{
	_offloadEndTime = timeGetTime();
	DWORD totalTime = _offloadEndTime - _offloadStartTime;
	double fps = ((_renderedFrames - 147)*1000) / totalTime;
	char result[100];
	int size = sprintf_s(result,"Average FPS:%f\nTotal Frames:%d\nTime elapsed:%d\r\t",fps,_renderedFrames-147,totalTime);

	KahawaiWriteFile("KahawaiFPS.log", result,size);

}

char* Kahawai::GetDemoFile()
{

	return _demoFile;
}

Kahawai* kahawai;
char				g_resultsPath[200]; //Path to save the captured frames

//////////////////////////////////////////////////////////////////////////
// Associated utility methods
//////////////////////////////////////////////////////////////////////////

bool CreateKahawaiThread(LPTHREAD_START_ROUTINE function, void* instance)
{
	DWORD ThreadID;
	HANDLE thread = CreateThread(NULL,0,function, instance, 0, &ThreadID);

	if(thread==NULL)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Debug
//////////////////////////////////////////////////////////////////////////


#endif