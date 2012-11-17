#ifdef KAHAWAI
#ifndef KAHAWAI_H
#define KAHAWAI_H

#include "kahawaiBase.h"

#include "Capturer.h"

class ConfigReader;

class Kahawai
{
public:
	//////////////////////////////////////////////////////////////////////////
	//Kahawai Public interface
	//////////////////////////////////////////////////////////////////////////
	~Kahawai(void);

	static Kahawai*		LoadFromFile(); //Factory to create Kahawai instance
	virtual bool		StartOffload();
	virtual bool		Offload();
	virtual bool		ShouldSkip(); //Should the frame be rendered (used in I/P rendering)
	virtual bool		StopOffload();
	virtual bool		IsHD()=0; //Should render in hi or lo quality
	bool				IsOffloading();

	//Timer Handling
	int					Sys_Milliseconds();
	
	//Debug: Demo file to run
	char*				GetDemoFile();

	//////////////////////////////////////////////////////////////////////////

protected:
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Kahawai Lifecycle / Pipeline
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Kahawai Pipeline
	virtual bool		Initialize();
	virtual bool		Capture(int width, int height);
	virtual bool		Transform(int width, int height);
	//Encode and Send implemented by servers
	//Decode and Show implemented by clients

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	//Internal methods
	//////////////////////////////////////////////////////////////////////////


	//Post-initialization steps
	void				SetReader(ConfigReader* reader);
	void				SetCaptureMode(CAPTURE_MODE mode);

	//Network Setup
	bool				InitNetworking();

	//Accessors
	int					GetHeight();
	int					GetWidth();
	int					GetDisplayedFrames();
	//Instrumentation
	void				LogFPS();

	//Asynchronous pipeline start point

	static DWORD WINAPI StartPipeline(void* Param);
	virtual void		OffloadAsync()=0; //Must be implemented only by subclasses

	//Keep constructor protected, to only allow creation through factory
	Kahawai();

	//////////////////////////////////////////////////////////////////////////
	// Kahawai Common state
	//////////////////////////////////////////////////////////////////////////

	//Kahawai global settings
	ENCODING_MODE		_mode;
	CAPTURE_MODE		_captureMode;
	char				_demoFile[30]; //Name of the reply to run if applicable.

	//Kahawai General Network settings
	int					_serverPort;

	int					_height; //These are the screen dimensions. Not the client rendered dimension
	int					_width;
	int					_bitDepth; //Bit depth in bytes (3 = 24 bits)

	//Kahawai system general state
	bool				_offloading;
	int					_fps; //target fps of the game. Normally 60
	int					_timeStep; // Number of milliseconds per RENDERED frame. 
	int					_renderedFrames; //total frames rendered so far

	//Kahawai performance profile information
	unsigned long		_offloadStartTime;
	unsigned long		_offloadEndTime;

	//Kahawai captured framebuffer
	uint8_t*			_sourceFrame;
	ConfigReader*		_configReader;

	//Kahawai capturer
	Capturer*			_capturer;

	//Capture stage synchronization
	CONDITION_VARIABLE _captureFullCV;
	CONDITION_VARIABLE _captureReadyCV;

	CRITICAL_SECTION _frameBufferCS;
	//Transform phase synchronization
	bool _frameInProcess;

	// Lib sws_scale context. (Implementation tied to libx264 because of the datatypes used for pic handling)
	SwsContext*			_convertCtx; //Each implementation class has to initialize this one if resizing is desired
	x264_picture_t*		_transformPicture; //Picture structure used for manipulations such as colorspace changes.

	//////////////////////////////////////////////////////////////////////////




};
extern Kahawai*			kahawai;




#endif
#endif