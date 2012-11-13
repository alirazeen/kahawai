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

	if(!_master)
	{	//Re-Initialize sws scaling context if slave
		delete[] _sourceFrame;
		_convertCtx = sws_getContext(_clientWidth,_clientHeight,PIX_FMT_RGB24, _width, _height,PIX_FMT_YUV420P, SWS_FAST_BILINEAR,NULL,NULL,NULL);
		_sourceFrame = new uint8_t[_clientWidth*_clientWidth*SOURCE_BITS_PER_PIXEL];
	}

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
	//Create socket to client (if master)
	if(_master)
		_socketToClient = CreateSocketToClient(_serverPort);

	if(_socketToClient==INVALID_SOCKET)
	{
		KahawaiLog("Unable to create connection to client", KahawaiError);
		return;
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
	//Synchronize both processes
	SyncCopies();
	return KahawaiServer::Transform(_width, _height);
}


int DeltaServer::Encode(void** compressedFrame)
{
	if(_master)
	{
		return _encoder->Encode(_transformPicture,*compressedFrame,Delta, _mappedBuffer);
	}
	else
	{
		compressedFrame = (void**) &_transformPicture;
		return YUV420pBitsPerPixel(_width,_height);
	}
}

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

	}
	else //Slave
	{
		//Copy the low fidelity capture to shared memory
		memcpy(_mappedBuffer,compressedFrame,frameSize);
	}

	return false;
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
	int size = YUV420pBitsPerPixel(_clientWidth , _clientHeight);
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


	WaitForSingleObject(_mutex,INFINITE);

	char* b = (char*) MapViewOfFile(_map,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		16);

	if(strcmp(kahawaiMaster,b)!=0)
	{
		strcpy(b,kahawaiMaster);
		_master = true;
	}
	else
	{
		_master = false;
	}

	UnmapViewOfFile(b);
	ReleaseMutex(_mutex);

	DWORD access = _master? FILE_MAP_READ: FILE_MAP_WRITE;
	_mappedBuffer = (byte*) MapViewOfFile(_map,
		access,
		0,
		0,
		YUV420pBitsPerPixel(_clientWidth, _clientHeight));

	return true;
}


void DeltaServer::SyncCopies()
{
	if(_master)
	{
		//signal and wait for slave
		SetEvent(_masterBarrier);
		WaitForSingleObject(_slaveBarrier, INFINITE);
	}
	else
	{
		//signal and wait for master
		SetEvent(_slaveBarrier);
		WaitForSingleObject(_masterBarrier,INFINITE);
	}
}

bool DeltaServer::IsHD()
{
	//Only master renders in HD
	return _master;
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