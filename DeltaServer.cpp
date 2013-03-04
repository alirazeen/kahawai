#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "DeltaServer.h"

//Supported encoders
#include "X264Encoder.h"

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
	_inputHandler = new InputHandlerServer(_serverPort+10, _gameName);
#endif

	if(!_master)
	{	//Re-Initialize sws scaling context if slave
		delete[] _sourceFrame;
		_convertCtx = sws_getContext(_clientWidth,_clientHeight,PIX_FMT_BGRA, _width, _height,PIX_FMT_YUV420P, SWS_FAST_BILINEAR,NULL,NULL,NULL);
		_sourceFrame = new uint8_t[_clientWidth*_clientWidth*SOURCE_BITS_PER_PIXEL];
	}

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
		bool connection = true;

#ifndef NO_HANDLE_INPUT
		connection = _inputHandler->Connect();
#endif
		_socketToClient = CreateSocketToClient(_serverPort);

		if(_socketToClient==INVALID_SOCKET || !connection)
		{
			KahawaiLog("Unable to create connection to client", KahawaiError);
			return;
		}

	}


	KahawaiServer::OffloadAsync();

}


/**
 * Overrides Kahawai::Transform because it needs to synchronize
 * the master and slave processes first.
 * @param width the target width
 * @param height the target height
 * @see Kahawai::Transform
 * @return true if the transformation was successful
 */
bool DeltaServer::Transform(int width, int height)
{
	return KahawaiServer::Transform(_width, _height);
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
	if(_master)
	{
		int result = 0;

		LogYUVFrame(_renderedFrames,"source",_renderedFrames,(char*)_transformPicture->img.plane[0],_width,_height);
	
		//Wait for the slave copy to finish writing the low quality version to shared memory
		WaitForSingleObject(_masterBarrier, INFINITE);
			result = _encoder->Encode(_transformPicture,transformedFrame,Delta, _mappedBuffer);
		SetEvent(_slaveBarrier);

		return result;
	}
	else
	{
		*transformedFrame =  _transformPicture->img.plane[0];
		return YUV420pBitsPerPixel(_width,_height);
	}
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

		SetEvent(_masterBarrier);
		WaitForSingleObject(_slaveBarrier, INFINITE);
	}

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
void* DeltaServer::HandleInput(void*)
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
	return 3;
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
	_map(NULL)
{

}


DeltaServer::~DeltaServer(void)
{

}
#endif