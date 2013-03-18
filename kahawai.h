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
	//Kahawai Public interface (Must be implemented by derived classes)
	//////////////////////////////////////////////////////////////////////////
	//Input Handling
	virtual void*		HandleInput(void* input)=0;
	virtual int			GetFirstInputFrame()=0; //returns the number of the first frame to receive input
	virtual int			GetDisplayedFrames()=0;
	virtual bool		IsInputSource()=0;

	//Asynchronous Kahawai thread (Usually starts with "Transform")
	virtual void		OffloadAsync()=0;
	//Rendering Settings
	virtual bool		IsHD()=0; //Should render in hi or lo quality

	//////////////////////////////////////////////////////////////////////////
	// Kahawai Public concrete methods
	//////////////////////////////////////////////////////////////////////////

	~Kahawai(void);

	static Kahawai*		LoadFromFile(); //Factory to create Kahawai instance
	virtual bool		StartOffload();
	virtual bool		Offload();
	virtual bool		ShouldSkip(); //Should the frame be rendered (used in I/P rendering)
	virtual bool		StopOffload();
	bool				IsOffloading();

	//Instrumentation at Doom 3 engine
	virtual void			FrameStart()=0;
	virtual void			FrameEnd()=0;

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
	virtual bool		Finalize(); //Cleanup specially for locks and threads


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	//Internal methods
	//////////////////////////////////////////////////////////////////////////


	//Post-initialization steps
	void				SetReader(ConfigReader* reader);
	void				SetCaptureMode(CAPTURE_MODE mode);

	//Accessors
	int					GetHeight();
	int					GetWidth();
	bool				ShouldHandleInput();


	//Instrumentation
	void				LogFPS();

	//Asynchronous pipeline start point

	static DWORD WINAPI StartPipeline(void* Param);

	//Keep constructor protected, to only allow creation through factory
	Kahawai();





	//////////////////////////////////////////////////////////////////////////
	// Kahawai Common state
	//////////////////////////////////////////////////////////////////////////

	//Kahawai global settings
	ENCODING_MODE		_mode;
	CAPTURE_MODE		_captureMode;

	//Kahawai game settings
	char				_gameName[100]; //Name of the game being offloaded
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


	// Lib sws_scale context. (Implementation tied to libx264 because of the datatypes used for pic handling)
	SwsContext*			_convertCtx; //Each implementation class has to initialize this one if resizing is desired
	x264_picture_t*		_transformPicture; //Picture structure used for manipulations such as colorspace changes.

	//Cleanup state
	bool				_finished;

	//Debug Settings
	bool				_saveCaptures; //Used for measuring quality. save snapshots to disk

private:
	//Capture stage synchronization
	CONDITION_VARIABLE	_captureFullCV;
	CONDITION_VARIABLE	_captureReadyCV;

	CRITICAL_SECTION	_frameBufferCS;

	//Transform phase synchronization
	bool				_frameInProcess;


	//////////////////////////////////////////////////////////////////////////


};
extern Kahawai*			kahawai;

#endif
#endif
