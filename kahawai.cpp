#include "kahawai.h"
#include<iostream>
#include<fstream>
#include<string>

using namespace std;

KAHAWAI_MODE kahawaiProcessId = Undefined;
HANDLE kahawaiMutex = NULL;
HANDLE kahawaiMap;
HANDLE kahawaiSlaveBarrier;
HANDLE kahawaiMasterBarrier;
const char kahawaiMaster[8] = "leader\n";
int kahawaiLoVideoHeight;
int kahawaiLoVideoWidth;
int kahawaiHiVideoHeight;
int kahawaiHiVideoWidth;
char kahawaiServerIP[75];
int kahawaiServerPort;

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
#ifdef NO_NETWORKING
	return false; //cannot do KAHAWAI without networking
#endif


	ifstream kahawaiiConfigFile;
	int server;
	string line;

	char loVideoMode;
	char hiVideoMode;

	//TODO:Something much fancier could be done here. but this will	if(fileSystem->IsInitialized())
	{
	

		kahawaiiConfigFile.open("kahawai.cfg",ios::in);
		if(!kahawaiiConfigFile.is_open())
		{
			return false;
		}

		getline(kahawaiiConfigFile,line);
		server = atoi(line.c_str());

		if(!server==1)
		{
			kahawaiProcessId = Client;
			getline(kahawaiiConfigFile,line);
			strcpy_s(kahawaiServerIP,line.c_str());

			getline(kahawaiiConfigFile,line);
			kahawaiServerPort=atoi(line.c_str());

			kahawaiiConfigFile.close();
			return true;
		}
		else //Otherwise is server, and can be either master or slave process. Determine through mapping
		{
			//read the configuration for the lo and hi resolution modes

			getline(kahawaiiConfigFile,line);
			kahawaiServerPort=atoi(line.c_str());

			getline(kahawaiiConfigFile,line);
			kahawaiLoVideoWidth=atoi(line.c_str());

			getline(kahawaiiConfigFile,line);
			kahawaiLoVideoHeight=atoi(line.c_str());
			
			getline(kahawaiiConfigFile,line);
			kahawaiHiVideoWidth=atoi(line.c_str());

			getline(kahawaiiConfigFile,line);
			kahawaiHiVideoHeight=atoi(line.c_str());

			kahawaiiConfigFile.close();
			//Initialize file mapping to share low resolution image between both processes
			return InitMapping((kahawaiLoVideoHeight * kahawaiLoVideoWidth * 3)/2);
		}
		return 0;
	}

}
