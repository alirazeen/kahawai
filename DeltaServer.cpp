#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "DeltaServer.h"

//Supported encoders
#include "X264Encoder.h"


bool DeltaServer::isClient() {
	return false;
}

bool DeltaServer::isMaster() {
	return theMaster;
}

bool DeltaServer::isSlave() {
	return theSlave;
}

//////////////////////////////////////////////////////////////////////////
// Basic Delta encoding transformation
//////////////////////////////////////////////////////////////////////////
byte Delta(byte hi, byte lo)
{
	int resultPixel = ((hi - lo) / 2) + 127;

	if (resultPixel > 255)
		resultPixel = 255;

	if (resultPixel < 0)
		resultPixel = 0;

	return (byte)resultPixel;
}


/////////////////////////////////////////////////////////////////////////
// Lifecycle methods
//////////////////////////////////////////////////////////////////////////


bool DeltaServer::Initialize()
{
	//Call superclass
	if(!KahawaiServer::Initialize())
		return false;

	//Read client's resolution. (Can be lower than the server's. Interpolation occurs to match deltas)
	_clientWidth = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_WIDTH);
	_clientHeight = _configReader->ReadIntegerValue(CONFIG_DELTA,CONFIG_HEIGHT);

	//Initialize Memory FileMapping. Decide roles while at it
	if(!InitMapping())
	{
		KahawaiLog("Unable to create memory file map", KahawaiError);
		return false;
	}

	//Initialize encoder
	_encoder = new X264Encoder(_height,_width,_fps,_crf,_preset);

	//Initialize input handler
#ifndef NO_HANDLE_INPUT
	_inputHandler = new InputHandlerServer(_serverPort+PORT_OFFSET_INPUT_HANDLER, _gameName);
#endif

	char* measurement_file_name;
	if (_master)
	{
		theMaster = true;
		theSlave = false;
		measurement_file_name = "delta_server_master.csv";

	} else
	{	
		theMaster = false;
		theSlave = true;
		//Re-Initialize sws scaling context if slave
		delete[] _sourceFrame;
		_convertCtx = sws_getContext(_clientWidth,_clientHeight,PIX_FMT_BGRA, _width, _height,PIX_FMT_YUV420P, SWS_FAST_BILINEAR,NULL,NULL,NULL);
		_sourceFrame = new uint8_t[_clientWidth*_clientWidth*SOURCE_BITS_PER_PIXEL];

		measurement_file_name = "delta_server_slave.csv";
	}

#ifndef MEASUREMENT_OFF
	_measurement = new Measurement(measurement_file_name);
	KahawaiServer::SetMeasurement(_measurement);
	_inputHandler->SetMeasurement(_measurement);
	_encoder->SetMeasurement(_measurement);
#endif

	InitializeCriticalSection(&_inputSocketCS);
	InitializeConditionVariable(&_inputSocketCV);

	return true;
}

bool DeltaServer::Finalize()
{
	KahawaiServer::Finalize();

	//Unlock waiting threads

	if(_master)
		SetEvent(_slaveBarrier);
	else
		SetEvent(_masterBarrier);

	return true;
}

bool DeltaServer::StartOffload()
{
	bool result = KahawaiServer::StartOffload();

#ifndef NO_HANDLE_INPUT
	//Make sure the input handler is connected first before we allow 
	//the delta master and slave to continue. The synchronization logic
	//here is complicated because:
	//
	// 1) There is a delta master and there is a delta slave
	// 2) Only the master connects to the client input handler
	// 3) The slave has to wait for the master to finish connecting
	// 4) The slave and the master are _separate_ class instances
	// 5) As a consequence of 4), there are twp independent critical sections with the 
	//    same name, _inputSocketCS, and also independent condition variables
	// 6) The one thing that both the master and the slave share is the _masterInputEvent HANDLE

	if (_master && result && !_inputConnectionDone)
	{
		EnterCriticalSection(&_inputSocketCS);
		{
			while(!_inputConnectionDone)
				SleepConditionVariableCS(&_inputSocketCV, &_inputSocketCS, INFINITE);
		}
		SetEvent(_masterInputEvent);
		LeaveCriticalSection(&_inputSocketCS);

		result = _inputHandler->IsConnected();
	} else if (!_master && result)
	{
		EnterCriticalSection(&_inputSocketCS);
		{
			while(!_masterInputReady)
				SleepConditionVariableCS(&_inputSocketCV, &_inputSocketCS, INFINITE);
		}
		LeaveCriticalSection(&_inputSocketCS);
	}
#endif

	return result;
}

/**
 * Executes the server side of the pipeline
 * Defines initialization steps before first frame is offloaded
 * Transform->Encode->Send
 */
void DeltaServer::OffloadAsync()
{
	//First perform final initialization step
	//Create socket and input connection to client (if master)
	if(_master)
	{
		bool connected = ConnectToClientDecoder();
		if (!connected)
		{
			KahawaiLog("Unable to connect client decoder\n", KahawaiError);
			return;
		}

#ifndef NO_HANDLE_INPUT
		EnterCriticalSection(&_inputSocketCS);
		{
			_inputHandler->Connect();
			_inputConnectionDone = true;
		}
		WakeConditionVariable(&_inputSocketCV);
		LeaveCriticalSection(&_inputSocketCS);

		if(!_inputHandler->IsConnected())
		{
			KahawaiLog("Unable to start input handler", KahawaiError);
			_offloading = false;
			return;
		}
#endif

	} else
	{
#ifndef NO_HANDLE_INPUT
		EnterCriticalSection(&_inputSocketCS);
		{
			WaitForSingleObject(_masterInputEvent, INFINITE);
			_masterInputReady = true;
		}
		WakeConditionVariable(&_inputSocketCV);
		LeaveCriticalSection(&_inputSocketCS);
#endif
	}


	KahawaiServer::OffloadAsync();

}

bool DeltaServer::Capture(int width, int height, void* args)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::CAPTURE_BEGIN, _gameFrameNum);
#endif

	bool result = KahawaiServer::Capture(width, height, args);

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::CAPTURE_END, _gameFrameNum);
#endif

	return result;
}

/**
 * Overrides Kahawai::Transform because it needs to synchronize
 * the master and slave processes first.
 * @param width the target width
 * @param height the target height
 * @see Kahawai::Transform
 * @return true if the transformation was successful
 */
bool DeltaServer::Transform(int width, int height, int frameNum)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_BEGIN, frameNum);
#endif

	bool result = KahawaiServer::Transform(_width, _height, _kahawaiFrameNum);

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::TRANSFORM_END, frameNum);
#endif

	return result;
}

/**
 * The master combines the low definition from the slave with its own 
 * and encodes it
 * The slaves writes its low definition version to shared memory
 * @param transformedFrame a pointer to the transformedFrame
 * @see KahawaiServer::Encode
 * @return
 */
int DeltaServer::Encode(void** transformedFrame)
{
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::ENCODE_BEGIN, _kahawaiFrameNum);
#endif

	int result = 0;
	if(_master)
	{
		LogYUVFrame(_renderedFrames,"source",_renderedFrames,(char*)_transformPicture->img.plane[0],_width,_height);
	
		//Wait for the slave copy to finish writing the low quality version to shared memory
		WaitForSingleObject(_masterBarrier, INFINITE);

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase("ENCODE_MASTER_AFTER_WAIT", _kahawaiFrameNum);
#endif

		result = _encoder->Encode(_transformPicture,transformedFrame,Delta, _mappedBuffer);
		SetEvent(_slaveBarrier);

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase("ENCODE_MASTER_SET_EVENT", _kahawaiFrameNum);
#endif
	}
	else
	{
		*transformedFrame =  _transformPicture->img.plane[0];
		result = YUV420pBitsPerPixel(_width,_height);
	}

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::ENCODE_END, _kahawaiFrameNum);
#endif


	return result;
}

/**
 * Sends the encoded frame to the client 
 * @param compressed frame, a pointer to the compressedFrame array
 * @param frameSize the size of the array
 * @see KahawaiServer::Send
 * @return true if the operation succeeded, false otherwise
 */
bool DeltaServer::Send(void** compressedFrame, int frameSize)
{

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::SEND_BEGIN, _kahawaiFrameNum);
#endif

	if(_master)
	{
		//Send the encoded delta to the client
		if(send(_socketToClient,(char*) *compressedFrame,frameSize,0)==SOCKET_ERROR)
		{
			KahawaiLog("Unable to send frame to client", KahawaiError);
			return false;
		}

		LogVideoFrame(_saveCaptures,"transferred","deltaMovie.h264",(char*)*compressedFrame,frameSize);
	}
	else //Slave
	{

		//Copy the low fidelity capture to shared memory
		memcpy(_mappedBuffer,(char*) *compressedFrame,frameSize);

#ifndef MEASUREMENT_OFF
		_measurement->AddPhase("SEND_SLAVE_BEFORE_WAIT", _kahawaiFrameNum);
#endif

		SetEvent(_masterBarrier);
		WaitForSingleObject(_slaveBarrier, INFINITE);
	}

#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::SEND_END, _kahawaiFrameNum);
#endif

	return true;
}


//////////////////////////////////////////////////////////////////////////
// Support methods
//////////////////////////////////////////////////////////////////////////

/**
 * Inits File Mapping between the Delta Hi and the Delta Lo processes
 * @param the size of the mapping
 * @return
 */
bool DeltaServer::InitMapping()
{
	int inputOffset = YUV420pBitsPerPixel(_clientWidth , _clientHeight);
	int size = inputOffset + KAHAWAI_INPUT_COMMAND_BUFFER;
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
	_map = CreateFileMapping
		(sharedFile,
		NULL,
		PAGE_READWRITE,
		0,
		size,
		_T("SharedFile"));

	if(!_map)
		return false;

	//Define a Mutex for accessing the file map.
	_mutex = CreateMutex(
		NULL,
		FALSE,
		_T("FILE MUTEX"));

	if(!_mutex)
		return false;

	_slaveBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiSlave");

	_masterBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiMaster");

	//Define events for accessing the input section of the file map

	_slaveInputEvent = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiSlaveInput");

	_masterInputEvent = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiMasterInput");

	WaitForSingleObject(_mutex,INFINITE);

	char* b = (char*) MapViewOfFile(_map,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		size);

	if(strcmp(kahawaiMaster,b)!=0)
	{
		memset(b,0,size);
		strcpy(b,kahawaiMaster);
		_master = true;
	}
	else
	{
		_master = false;
	}

	UnmapViewOfFile(b);
	ReleaseMutex(_mutex);

	DWORD access = FILE_MAP_ALL_ACCESS;
	_mappedBuffer = (byte*) MapViewOfFile(_map,
		access,
		0,
		0,
		size);

	_sharedInputBuffer = _mappedBuffer + inputOffset;

	return true;
}

/**
 * Returns whether this instance should use high or low quality settings 
 * @return the quality of the settings to be used
 */
bool DeltaServer::IsHD()
{
	//Only master renders in HD
	return _master;
}

//////////////////////////////////////////////////////////////////////////
// Input Handling
//////////////////////////////////////////////////////////////////////////

/**
 * Receives input from the client and applies it to the server state
 * @param Server input is ALWAYS discarded
 * @return the command received from the client for the current frame
 */
void* DeltaServer::HandleInput()
{

	if(!ShouldHandleInput())
		return _inputHandler->GetEmptyCommand();

	if(_master)
	{
		int length = _inputHandler->GetCommandLength();
		char* cmd = (char*) _inputHandler->ReceiveCommand();
		//Send the input to the slave copy
		//TODO: The barrier may not be enough synchronization
		//Check this if synchronization issues arise. 
		//Performance may be hurt if a stronger method is used
		memcpy(_sharedInputBuffer,cmd,length);
		SetEvent(_masterInputEvent);
		WaitForSingleObject(_slaveInputEvent, INFINITE);

#ifndef MEASUREMENT_OFF
		_measurement->InputProcessed(_numInputProcessed, _gameFrameNum);
#endif
		_numInputProcessed++;

		return cmd;
	}
	else
	{
		void* result = NULL;
		
		WaitForSingleObject(_masterInputEvent, INFINITE);
		result = _sharedInputBuffer;
		SetEvent(_slaveInputEvent);

		return result;
	}
}



int DeltaServer::GetFirstInputFrame()
{
	//TODO: Need to give a real value based on profiling
	return FRAME_GAP;
}

bool DeltaServer::ConnectToClientDecoder()
{
	//TODO: POSSIBLE MEMLEAK BECAUSE WE ARE NOT DELETING encodedFrame??
	void *encodedFrame = NULL;
	int frameSize = _encoder->GetBlackFrame(_convertCtx, &encodedFrame);
	
	_socketToClient = CreateSocketToClient(_serverPort);
	if (_socketToClient==INVALID_SOCKET)
	{
		KahawaiLog("Unable to create connection to client in DeltaServer::OffloadAsync()", KahawaiError);
		return false;
	}

	if(send(_socketToClient,(char*) encodedFrame, frameSize, 0)==SOCKET_ERROR)
	{
		KahawaiLog("Unable to send frame to client", KahawaiError);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Constructor / Destructor
//////////////////////////////////////////////////////////////////////////

DeltaServer::DeltaServer(void)
	:KahawaiServer(),
	_slaveBarrier(NULL),
	_masterBarrier(NULL),
	_mappedBuffer(NULL),
	_master(false),
	_mutex(NULL),
	_map(NULL),
	_inputConnectionDone(false),
	_masterInputReady(false),
	_numInputProcessed(0)
{

}


DeltaServer::~DeltaServer(void)
{

}
#endif
