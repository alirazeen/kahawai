#include "kahawai.h"

KAHAWAI_MODE kahawaiProcessId = Undefined;
HANDLE kahawaiMutex = NULL;
HANDLE kahawaiMap;
HANDLE kahawaiSlaveBarrier;
HANDLE kahawaiMasterBarrier;
const char kahawaiMaster[8] = "leader\n";
int kahawaiLoVideoMode;
int kahawaiHiVideoMode;
int kahawaiIFrameSkip; //to be used in I-Frame mode

/*
=================
InitMapping
=================
*/
bool InitMapping(int size)
{
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
	kahawaiMap = CreateFileMapping
		(sharedFile,
		NULL,
		PAGE_READWRITE,
		0,
		size,
		_T("SharedFile"));

	if(!kahawaiMap)
		return false;

	//Define a Mutex for accessing the file map.
	kahawaiMutex = CreateMutex(
		NULL,
		FALSE,
		_T("FILE MUTEX"));

	if(!kahawaiMutex)
		return false;

	kahawaiSlaveBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiSlave");

	kahawaiMasterBarrier = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		"KahawaiiMaster");


	WaitForSingleObject(kahawaiMutex,INFINITE);

	char* b = (char*) MapViewOfFile(kahawaiMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		16);

	if(strcmp(kahawaiMaster,b)!=0)
	{
		strcpy(b,kahawaiMaster);
		kahawaiProcessId = Master;
	}
	else
	{
		kahawaiProcessId = Slave;
	}

	UnmapViewOfFile(b);
	ReleaseMutex(kahawaiMutex);

	return true;
}

bool InitKahawai()
{
	idFile* kahawaiiConfigFile=NULL;
	char role;

	char loVideoMode;
	char hiVideoMode;

	//TODO:Something much fancier could be done here. but this will	if(fileSystem->IsInitialized())
	{
	
		if(!fileSystem->IsInitialized())
			fileSystem->Init();

		if(!kahawaiiConfigFile)
		{
			kahawaiiConfigFile = fileSystem->OpenFileRead("kahawai.cfg");
			if(!kahawaiiConfigFile)
			{
				return false;
			}

		}

		kahawaiiConfigFile->ReadChar(role);

		if(role == Client)
		{
			kahawaiProcessId = Client;
			fileSystem->CloseFile(kahawaiiConfigFile);
			return true;
		}
		else //Otherwise is server, and can be either master or slave process. Determine through mapping
		{
			//read the configuration for the lo and hi resolution modes
			kahawaiiConfigFile->ReadChar(loVideoMode);
			kahawaiiConfigFile->ReadChar(hiVideoMode);

			kahawaiLoVideoMode = atoi(&loVideoMode);
			kahawaiHiVideoMode = atoi(&hiVideoMode);

			fileSystem->CloseFile(kahawaiiConfigFile);
			//Initialize file mapping to share low resolution image between both processes
			return InitMapping(KAHAWAI_WIDTH[kahawaiLoVideoMode] * KAHAWAI_HEIGHT[kahawaiLoVideoMode] * 3);
		}

	}

}
